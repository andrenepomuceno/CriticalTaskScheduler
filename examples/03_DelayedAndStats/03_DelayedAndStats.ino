// CriticalTaskScheduler - Example 03: Delayed start + stats reporting
// Shows enableDelayed(), per-task stats, and Scheduler::printStats().
//
// Concepts:
//   * enableDelayed() to stagger task start times
//   * TaskStats struct (runs, avg, max, last, total)
//   * Scheduler::printStats(Print&)
//
// NOTE 1 — the delays below only SIMULATE callback work so the stats have
//   something to report. Real callbacks must be non-blocking; never call
//   delay() in production task callbacks.
// NOTE 2 — execution times use the default millis() clock, so they have 1 ms
//   resolution. The sub-millisecond fast() task therefore reports avg/max ~0;
//   that is expected, not a bug. Inject a micros()-based TimeProvider via
//   sched.setTimeProvider() if you need finer profiling.

#include <CriticalTaskScheduler.h>

TSScheduler sched;

void fast()  { delayMicroseconds(500); }   // ~0.5 ms work -> reads 0 ms (see NOTE 2)
void slow()  { delay(3); }                 // ~3 ms work
void mid()   { delayMicroseconds(1500); }  // ~1.5 ms work

TSTask fastTask("fast", 100, fast);
TSTask midTask("mid",   250, mid);
TSTask slowTask("slow", 500, slow);

unsigned long lastReport = 0;

void setup()
{
    Serial.begin(115200);
    delay(200);

    sched.addTask(&fastTask);
    sched.addTask(&midTask);
    sched.addTask(&slowTask);

    // Stagger start times so they don't all fire on the same loop tick.
    fastTask.enableDelayed(0);
    midTask.enableDelayed(50);
    slowTask.enableDelayed(150);
}

void loop()
{
    sched.execute();

    if (millis() - lastReport >= 5000)
    {
        lastReport = millis();
        Serial.println(F("--- task stats ---"));
        sched.printStats(Serial);

        // Or read structured stats yourself:
        const taskscheduler::TaskStats s = midTask.stats();
        Serial.print(F("mid avg ms = "));
        Serial.println(s.avgExecutionTime, 3);
    }
}
