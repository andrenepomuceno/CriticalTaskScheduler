# API Reference

All public symbols live in namespace `taskscheduler`. The convenience aliases
`TSTask`, `TSScheduler`, and `TSFreeRTOSCriticalRunner` are provided at
global scope unless you define `CRITICALTASKSCHEDULER_NO_GLOBAL_ALIASES`.

## `Task`

```cpp
Task(const char* name, unsigned long periodMs, TaskCallback cb, bool critical = false);
```

Construct a task. `name` must point to a string with static lifetime
(string literal recommended). `cb` is a plain function pointer
(`void(*)()`); capture-less lambdas are accepted.

| Method | Description |
|---|---|
| `void enable()` | Schedule first run *now*. |
| `void disable()` | Stop scheduling. Stats are preserved. |
| `void enableDelayed(unsigned long delayMs)` | Schedule first run at `now + delayMs`. |
| `bool isEnabled() const` | True if currently enabled. |
| `bool isCritical() const` | True if registered into the critical bucket. |
| `void setPeriod(unsigned long ms)` | Update period. Takes effect on the next reschedule. |
| `unsigned long getPeriod() const` | Current period. |
| `void setCallback(TaskCallback cb)` | Replace the callback at runtime. |
| `bool shouldRun(unsigned long now) const` | `enabled && now >= nextRunTime`. |
| `void run(unsigned long schedulerTime)` | Invoke callback, update stats and `nextRunTime`. Normally called by `Scheduler`. |
| `TaskStats stats() const` | Snapshot of all counters. |
| `const char* name() const` | Task name as passed to the constructor. |

### `TaskStats`

```cpp
struct TaskStats {
    const char*   name;
    bool          critical;
    bool          enabled;
    unsigned long period;
    unsigned long runs;
    unsigned long lastExecutionTime;   // ms
    unsigned long maxExecutionTime;    // ms
    unsigned long totalTime;           // ms
    float         avgExecutionTime;    // ms
    unsigned long nextRunTime;         // ms (epoch = boot)
};
```

## `Scheduler`

| Method | Description |
|---|---|
| `bool addTask(Task* task)` | Register a task in the appropriate bucket. Returns `false` if the bucket is full (`CRITICALTASKSCHEDULER_MAX_TASKS`, default 16). |
| `bool removeTask(Task* task)` | Unregister. Returns `true` if found. |
| `void execute()` | Run **one** earliest-due background task. |
| `void executeCritical()` | Run **every** due critical task. |
| `void enableAll()` / `disableAll()` | Bulk enable/disable. |
| `size_t taskCount() const` / `criticalTaskCount() const` | Bucket sizes. |
| `Task* taskAt(size_t)` / `criticalTaskAt(size_t)` | Random access for inspection. |
| `void printStats(Print& out) const` | One-line-per-task report to any `Print` (e.g. `Serial`). |
| `void setTimeProvider(TimeProvider tp)` | Inject a custom clock. Pass `nullptr` to restore `millis()`. |

## `FreeRTOSCriticalRunner` (requires FreeRTOS, `CRITICALTASKSCHEDULER_HAS_FREERTOS`)

Auto-detected on **ESP32** (`ARDUINO_ARCH_ESP32`), **RP2040** (`ARDUINO_ARCH_RP2040`, arduino-pico core), **nRF52** (`ARDUINO_ARCH_NRF52`, Adafruit core), **STM32** (`ARDUINO_ARCH_STM32`, STM32duino + FreeRTOS middleware), and **Teensy 4.x** (`TEENSYDUINO`, IMXRT). On any other platform with FreeRTOS, set `-D CRITICALTASKSCHEDULER_HAS_FREERTOS=1` in your build flags.

```cpp
FreeRTOSCriticalRunner(Scheduler& sched,
                       uint32_t      stackSize = 4096,
                       UBaseType_t   priority  = configMAX_PRIORITIES - 1,
                       uint32_t      tickMs    = 10);
```

Creates a FreeRTOS task that calls `sched.executeCritical()` every `tickMs`.

| Method | Description |
|---|---|
| `bool start()` | Spawn the FreeRTOS task. Idempotent. |
| `void stop()` | Delete the task. |
| `bool isRunning() const` | Whether the task is currently active. |
| `uint32_t getFreeStack() const` | High-water mark (bytes). 0 if not running. |

## Compile-time configuration

| Macro | Default | Effect |
|---|---|---|
| `CRITICALTASKSCHEDULER_MAX_TASKS` | 16 | Per-bucket capacity (background and critical). Override via `-D CRITICALTASKSCHEDULER_MAX_TASKS=64`. |
| `CRITICALTASKSCHEDULER_HAS_FREERTOS` | auto-detected | Set to `1` to enable `FreeRTOSCriticalRunner`. Auto-enabled on ESP32, RP2040 (arduino-pico), nRF52 (Adafruit), STM32 (STM32duino), and Teensy 4.x. Set manually on any other FreeRTOS-capable platform. |
