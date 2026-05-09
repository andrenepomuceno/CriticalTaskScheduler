// TaskScheduler - Example 01: Basic blink + heartbeat
// Two cooperative background tasks running off a single Scheduler.
//
// Concepts:
//   * Creating Tasks
//   * addTask + enableAll
//   * Scheduler::execute() in loop()
//
// Expected output (on Serial @ 115200):
//   alive
//   alive
//   ...
// while LED_BUILTIN toggles every 500 ms.

#include <TaskScheduler.h>

TSScheduler sched;

void blink()
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void heartbeat()
{
    Serial.println(F("alive"));
}

TSTask blinkTask("blink", 500, blink);
TSTask heartbeatTask("heartbeat", 1000, heartbeat);

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    sched.addTask(&blinkTask);
    sched.addTask(&heartbeatTask);
    sched.enableAll();
}

void loop()
{
    sched.execute();
}
