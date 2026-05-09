// TaskScheduler - Example 02: Critical vs Background tasks (ESP32)
// Demonstrates the two scheduling modes and the optional FreeRTOSCriticalRunner.
//
// Critical tasks run on a dedicated high-priority FreeRTOS task at a 10 ms tick
// (every due critical task runs each tick). Background tasks share loop().
//
// Concepts:
//   * Critical-flag in Task constructor
//   * FreeRTOSCriticalRunner (ESP32 only)
//   * Reschedule semantics: critical = nominal cadence, background = absorbs jitter
//
// Build: any ESP32 board.

#include <TaskScheduler.h>

#if !FBSCHED_HAS_FREERTOS
#error "This example targets ESP32 (FreeRTOS-enabled cores)."
#endif

FBScheduler sched;
FBFreeRTOSCriticalRunner runner(sched, /*stackSize*/ 4096, configMAX_PRIORITIES - 1, /*tickMs*/ 10);

volatile uint32_t criticalCount = 0;
volatile uint32_t backgroundCount = 0;

void controlLoop()
{
    // Hot path: simulate a 50 ms control update.
    criticalCount++;
}

void telemetry()
{
    backgroundCount++;
    Serial.print(F("crit/sec="));
    Serial.print(criticalCount);
    Serial.print(F(" bg/sec="));
    Serial.println(backgroundCount);
    criticalCount = 0;
    backgroundCount = 0;
}

FBTask controlTask("control",   50,   controlLoop, /*critical*/ true);
FBTask telemetryTask("telemetry", 1000, telemetry);

void setup()
{
    Serial.begin(115200);
    delay(200);

    sched.addTask(&controlTask);
    sched.addTask(&telemetryTask);
    sched.enableAll();

    if (!runner.start())
    {
        Serial.println(F("Failed to start critical runner!"));
    }
}

void loop()
{
    sched.execute();
}
