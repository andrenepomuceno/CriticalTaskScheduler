// TaskScheduler - Example 03: Delayed start + stats reporting
// Shows enableDelayed(), per-task stats, and Scheduler::printStats().
//
// Concepts:
//   * enableDelayed() to stagger task start times
//   * TaskStats struct (runs, avg, max, last, total)
//   * Scheduler::printStats(Print&)

#include <TaskScheduler.h>

FBScheduler sched;

void fast()  { delayMicroseconds(500); }   // ~0.5 ms work
void slow()  { delay(3); }                 // ~3 ms work
void mid()   { delayMicroseconds(1500); }  // ~1.5 ms work

FBTask fastTask("fast", 100, fast);
FBTask midTask("mid",   250, mid);
FBTask slowTask("slow", 500, slow);

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
        const fbsched::TaskStats s = midTask.stats();
        Serial.print(F("mid avg ms = "));
        Serial.println(s.avgExecutionTime, 3);
    }
}
