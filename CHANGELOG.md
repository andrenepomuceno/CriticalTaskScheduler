# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-05-09

### Added
- Initial public release, extracted from the FullBot V2 firmware.
- `fbsched::Task` and `fbsched::Scheduler` classes (also exposed as
  `FBTask` and `FBScheduler` global aliases).
- Background pump (`Scheduler::execute()`) — runs the single earliest-due
  task per call to avoid loop monopolisation.
- Critical pump (`Scheduler::executeCritical()`) — runs all due critical
  tasks per call.
- Optional `fbsched::FreeRTOSCriticalRunner` (ESP32) that drives the
  critical pump on a dedicated FreeRTOS task at a configurable tick.
- Pluggable `TimeProvider` for tests / non-Arduino hosts.
- Per-task statistics: runs, last/avg/max/total execution time,
  next-run time.
- `Scheduler::printStats(Print&)` that streams a one-line-per-task report
  to any Arduino `Print` (e.g. `Serial`).
- Examples: `01_BasicBlink`, `02_CriticalVsBackground`, `03_DelayedAndStats`.
- Documentation: quick start, API reference, timing semantics, troubleshooting.
- Native Unity tests under `test/`.

[1.0.0]: https://github.com/andrenepomuceno/TaskScheduler/releases/tag/v1.0.0
