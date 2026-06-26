// CriticalTaskScheduler - Example 04: Runtime control + portable critical tasks
// Critical tasks WITHOUT FreeRTOS, plus reconfiguring tasks while they run.
//
// On MCUs that do not bundle FreeRTOS (e.g. AVR/UNO) you can still use critical
// tasks: just call Scheduler::executeCritical() yourself from loop() (or from a
// timer ISR). This sketch pumps BOTH buckets cooperatively from loop() and
// reconfigures tasks at runtime.
//
// Concepts:
//   * executeCritical() driven from loop() — no FreeRTOS required (portable)
//   * setPeriod()   — retune a task's interval at runtime
//   * setCallback() — swap a task's behaviour at runtime (capture-less lambda)
//   * disable() / removeTask() — stop and unregister a task
//   * addTask() return value — bucket-full guard
//
// Runs on any board, including AVR/UNO.

#include <CriticalTaskScheduler.h>

TSScheduler sched;

unsigned long beats = 0;

void heartbeat()
{
    Serial.print(F("beat "));
    Serial.println(++beats);
}

void chirp()
{
    Serial.println(F("chirp!"));
}

// Critical task: every time it is due, it runs on each executeCritical() call.
TSTask sampleTask("sample", 200, heartbeat, /*critical*/ true);
// Background task: at most one due task runs per execute() call.
TSTask blinkTask("blink", 1000, chirp);

void setup()
{
    Serial.begin(115200);
    delay(200);

    if (!sched.addTask(&sampleTask)) Serial.println(F("sample add failed"));
    if (!sched.addTask(&blinkTask))  Serial.println(F("blink add failed"));
    sched.enableAll();
}

void loop()
{
    // No FreeRTOS here: pump both buckets cooperatively from loop().
    sched.execute();          // one earliest-due background task
    sched.executeCritical();  // every due critical task

    // --- Runtime reconfiguration, driven by how many beats we've seen. ---

    // After 10 beats: double the sampling rate (200 ms -> 100 ms).
    if (beats == 10 && sampleTask.getPeriod() != 100)
    {
        sampleTask.setPeriod(100);
        Serial.println(F(">> sample period -> 100 ms"));
    }

    // After 20 beats: swap the background callback for a new one.
    static bool swapped = false;
    if (beats >= 20 && !swapped)
    {
        blinkTask.setCallback([]() { Serial.println(F("BLINK (new callback)")); });
        swapped = true;
        Serial.println(F(">> blink callback swapped"));
    }

    // After 30 beats: retire the critical task entirely.
    static bool retired = false;
    if (beats >= 30 && !retired)
    {
        sampleTask.disable();           // stop scheduling it...
        sched.removeTask(&sampleTask);  // ...and unregister it from the bucket
        retired = true;
        Serial.println(F(">> sample task removed"));
    }
}
