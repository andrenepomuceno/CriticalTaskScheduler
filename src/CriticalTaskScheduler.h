// CriticalTaskScheduler - Lightweight cooperative task scheduler for Arduino/ESP32.
// SPDX-License-Identifier: MIT
// Repository: https://github.com/andrenepomuceno/CriticalTaskScheduler
#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// Maximum number of tasks per scheduler instance (per category: background and critical).
// Override at compile time, e.g. -D CRITICALTASKSCHEDULER_MAX_TASKS=64
#ifndef CRITICALTASKSCHEDULER_MAX_TASKS
#define CRITICALTASKSCHEDULER_MAX_TASKS 16
#endif

// Auto-detect FreeRTOS support on platforms where it is bundled in the core.
// To opt in on any other platform that ships FreeRTOS headers, add:
//   -D CRITICALTASKSCHEDULER_HAS_FREERTOS=1
// to your build flags before including this header.
//
// NOTE: STM32 (STM32duino) and Teensy 4.x are NOT auto-detected because their
// Arduino cores do NOT bundle FreeRTOS. Users who have explicitly added a
// FreeRTOS library (e.g. stm32duino/STM32FreeRTOS) must opt in manually.
#ifndef CRITICALTASKSCHEDULER_HAS_FREERTOS
#  if defined(ARDUINO_ARCH_ESP32)  || \
      defined(ARDUINO_ARCH_RP2040) || \
      defined(ARDUINO_ARCH_NRF52)
#    define CRITICALTASKSCHEDULER_HAS_FREERTOS 1
#  else
#    define CRITICALTASKSCHEDULER_HAS_FREERTOS 0
#  endif
#endif

#if CRITICALTASKSCHEDULER_HAS_FREERTOS
#  if defined(ARDUINO_ARCH_ESP32)
     // ESP32 Arduino core nests FreeRTOS headers under freertos/
#    include <freertos/FreeRTOS.h>
#    include <freertos/task.h>
#  else
     // RP2040 (arduino-pico), nRF52 (Adafruit) and manually opted-in platforms
     // expose FreeRTOS headers at the top level.
#    include <FreeRTOS.h>
#    include <task.h>
#  endif
#endif

namespace taskscheduler {

// Plain function-pointer callback. Use a free function or capture-less lambda.
// State should live in globals or static class members; this keeps the library
// portable across AVR/ARM/ESP32 without std::function overhead.
using TaskCallback = void (*)();

// Pluggable time source. Default is Arduino's millis(); tests / non-Arduino
// hosts can inject a fake clock via Scheduler::setTimeProvider().
using TimeProvider = unsigned long (*)();

unsigned long defaultTimeProvider();

// Snapshot of a task's runtime statistics.
struct TaskStats
{
    const char *name;
    bool critical;
    bool enabled;
    unsigned long period;
    unsigned long runs;
    unsigned long lastExecutionTime;
    unsigned long maxExecutionTime;
    unsigned long totalTime;
    float avgExecutionTime;
    unsigned long nextRunTime;
};

class Scheduler;

class Task
{
public:
    // name      : pointer to a string with static lifetime (literal recommended).
    // periodMs  : execution interval in milliseconds.
    // cb        : callback (may be nullptr; setCallback() can install it later).
    // critical  : if true, task is registered into the critical bucket.
    Task(const char *name, unsigned long periodMs, TaskCallback cb, bool critical = false);

    void enable();
    void disable();
    void enableDelayed(unsigned long delayMs);

    bool isEnabled() const { return _enabled; }
    bool isCritical() const { return _critical; }

    void setPeriod(unsigned long ms) { _period = ms; }
    unsigned long getPeriod() const { return _period; }

    void setCallback(TaskCallback cb) { _callback = cb; }

    // True when enabled and the task is due. Uses rollover-safe arithmetic
    // ((long)(now - nextRunTime) >= 0), so it stays correct across the
    // ~49.7-day millis() wrap.
    bool shouldRun(unsigned long now) const;

    // Invoke the callback and update stats / next-run time.
    // schedulerTime is "now" as observed by the scheduler when picking this task;
    // critical tasks reschedule from schedulerTime (preserves nominal cadence),
    // background tasks reschedule from end-of-callback time (absorbs jitter).
    void run(unsigned long schedulerTime);

    TaskStats stats() const;
    const char *name() const { return _name; }

private:
    const char *_name;
    unsigned long _period;
    TaskCallback _callback;
    bool _critical;
    bool _enabled;

    unsigned long _nextRunTime;
    unsigned long _lastExecutionTime;
    unsigned long _maxExecutionTime;
    unsigned long _totalTime;
    unsigned long _runs;

    friend class Scheduler;

    // Test/customization seam: optional time provider override (set by Scheduler).
    TimeProvider _timeProvider;
    void useTimeProvider(TimeProvider tp) { _timeProvider = tp; }
    unsigned long now() const { return _timeProvider ? _timeProvider() : defaultTimeProvider(); }
};

class Scheduler
{
public:
    Scheduler();

    // Returns true if added; false if the appropriate bucket is full.
    bool addTask(Task *task);

    // Returns true if removed.
    bool removeTask(Task *task);

    // Background pump: runs at most ONE due task per call (the earliest-due).
    // Call this from loop(). Single-task-per-call avoids loop monopolisation
    // by long-running tasks and keeps cooperative jitter bounded.
    void execute();

    // Critical pump: runs ALL due critical tasks per call. Intended for a
    // dedicated high-priority thread (see FreeRTOSCriticalRunner) or a tight
    // loop on platforms without FreeRTOS.
    void executeCritical();

    void enableAll();
    void disableAll();

    size_t taskCount() const { return _count; }
    size_t criticalTaskCount() const { return _criticalCount; }

    Task *taskAt(size_t i) const { return i < _count ? _tasks[i] : nullptr; }
    Task *criticalTaskAt(size_t i) const { return i < _criticalCount ? _criticalTasks[i] : nullptr; }

    // Print one-line stats per task to any Arduino Print stream (Serial, etc.).
    void printStats(Print &out) const;

    // Inject a custom time source (default: millis()).
    void setTimeProvider(TimeProvider tp);
    TimeProvider timeProvider() const { return _now; }

private:
    Task *_tasks[CRITICALTASKSCHEDULER_MAX_TASKS];
    size_t _count;
    Task *_criticalTasks[CRITICALTASKSCHEDULER_MAX_TASKS];
    size_t _criticalCount;
    TimeProvider _now;
};

#if CRITICALTASKSCHEDULER_HAS_FREERTOS
// Optional helper that runs Scheduler::executeCritical() on a dedicated
// FreeRTOS task at a fixed tick interval.
//
// Supported platforms (auto-detected — FreeRTOS bundled in core):
//   - ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3  (ARDUINO_ARCH_ESP32)
//   - RP2040 / Raspberry Pi Pico  (ARDUINO_ARCH_RP2040, arduino-pico core)
//   - nRF52 / Adafruit Feather nRF52  (ARDUINO_ARCH_NRF52)
//
// Platforms requiring manual opt-in (FreeRTOS not bundled in core):
//   - STM32 / STM32duino: add stm32duino/STM32FreeRTOS to lib_deps, then
//     set -D CRITICALTASKSCHEDULER_HAS_FREERTOS=1 in build_flags.
//   - Teensy 4.x: add a FreeRTOS port to lib_deps, then set
//     -D CRITICALTASKSCHEDULER_HAS_FREERTOS=1 in build_flags.
//
// Any other platform with FreeRTOS: define CRITICALTASKSCHEDULER_HAS_FREERTOS=1
// and ensure <FreeRTOS.h> / <task.h> are on the include path.
//
// THREAD-SAFETY WARNING:
//   The runner invokes Scheduler::executeCritical() from a dedicated FreeRTOS
//   thread while loop() is still running. The Scheduler does NOT hold internal
//   locks. To avoid races on the critical task array and per-task stats:
//     1. Register and configure all critical tasks BEFORE calling start().
//     2. Do NOT call addTask/removeTask/enableAll/disableAll for critical
//        tasks once the runner is running. Either stop() the runner first,
//        or restrict mutations to background tasks.
//     3. Reading stats (printStats) is best-effort; values may appear
//        slightly inconsistent on 8-bit MCUs (non-atomic 32-bit reads).
class FreeRTOSCriticalRunner
{
public:
    FreeRTOSCriticalRunner(Scheduler &sched,
                           uint32_t stackSize = 4096,
                           UBaseType_t priority = configMAX_PRIORITIES - 1,
                           uint32_t tickMs = 10);

    bool start();
    void stop();
    bool isRunning() const { return _handle != nullptr; }

    // High-water mark of free stack (bytes), or 0 if not running.
    uint32_t getFreeStack() const;

private:
    static void runnerEntry(void *param);

    Scheduler &_sched;
    uint32_t _stackSize;
    UBaseType_t _priority;
    uint32_t _tickMs;
    TaskHandle_t _handle;
};
#endif // CRITICALTASKSCHEDULER_HAS_FREERTOS

} // namespace taskscheduler

// Convenience top-level aliases for users who don't want to type the namespace.
// Define CRITICALTASKSCHEDULER_NO_GLOBAL_ALIASES to suppress.
#ifndef CRITICALTASKSCHEDULER_NO_GLOBAL_ALIASES
using TSTask = ::taskscheduler::Task;
using TSScheduler = ::taskscheduler::Scheduler;
#if CRITICALTASKSCHEDULER_HAS_FREERTOS
using TSFreeRTOSCriticalRunner = ::taskscheduler::FreeRTOSCriticalRunner;
#endif
#endif
