# Troubleshooting

## "My task never runs"
- Did you `addTask()` *and* `enable()` (or `enableDelayed()`)?
- Are you actually calling `sched.execute()` in `loop()` (or the critical
  pump from somewhere)?
- Is the bucket full? `addTask()` returns `false` when more than
  `FBSCHED_MAX_TASKS` (default 16) tasks are registered. Override with
  `-D FBSCHED_MAX_TASKS=N`.

## "Background tasks fire later than expected"
By design, `execute()` runs **one** task per call. If five tasks are due
at the same instant, the fifth runs five `loop()` cycles later. Solutions:

- Make `loop()` tight — avoid blocking I/O outside callbacks.
- Promote the latency-sensitive task to **critical**.
- Stagger start times with `enableDelayed()` so deadlines don't overlap.

## "Critical task period drifts under load"
- Critical tasks reschedule from *scheduler time*, not callback end. If
  the callback regularly overruns the period, you'll see *back-to-back*
  invocations as the runner catches up. Either:
  - Reduce callback work, or
  - Increase the period, or
  - Increase the runner tick (`tickMs` constructor arg) — but never
    above the smallest task period.

## "FreeRTOSCriticalRunner won't start"
- Stack too small — bump `stackSize` (default 4096). On ESP32, complex
  tasks (logging, JSON, MQTT) often need 8192+.
- Out of RAM — check `ESP.getFreeHeap()`.
- Already running — `start()` is idempotent and returns `true` if a
  previous task handle exists.

## "I get a stack overflow in a critical task"
Watch `runner.getFreeStack()` in your supervisor task. If the high-water
mark approaches 0, increase `stackSize`.

## "Tasks behave correctly at boot but drift after a few hours"
`millis()` rolls over after ~49.7 days but the comparison
`now >= nextRunTime` uses unsigned arithmetic, so single-rollover events
are handled correctly. Issues usually point to a callback that itself
takes longer than `period` (see *period drift* above).

## "I want to test scheduling logic without an Arduino board"
Use `Scheduler::setTimeProvider()` to inject a fake clock. See the
`test/native/` directory for a Unity-based example that runs on the host.

## "Library name conflicts with `arkhipenko/TaskScheduler`"
This library is registered as **TaskScheduler** in both Arduino
Library Manager and PlatformIO Registry. Both libraries can coexist in
your `libraries/` folder.
