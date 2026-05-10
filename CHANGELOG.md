# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2] - 2026-05-09

### Fixed
- Removed STM32 (`ARDUINO_ARCH_STM32`) and Teensy 4.x (`TEENSYDUINO`) from
  the FreeRTOS auto-detection guard. Those cores do **not** bundle FreeRTOS,
  causing a fatal `FreeRTOS.h: No such file or directory` compile error for
  any user without an optional FreeRTOS library installed.
  Both platforms remain supported via manual opt-in:
  `-D CRITICALTASKSCHEDULER_HAS_FREERTOS=1` + add the FreeRTOS library to
  `lib_deps` (e.g. `stm32duino/STM32FreeRTOS` for STM32).
- Updated `FreeRTOSCriticalRunner` class comment and all documentation to
  correctly reflect auto-detected vs. opt-in platforms.

## [1.0.1] - 2026-05-09

### Fixed
- `FreeRTOSCriticalRunner` was limited to ESP32. Auto-detection now also covers
  RP2040 (arduino-pico core, `ARDUINO_ARCH_RP2040`) and nRF52 (Adafruit core,
  `ARDUINO_ARCH_NRF52`). Any other FreeRTOS-capable platform can opt in with
  `-D CRITICALTASKSCHEDULER_HAS_FREERTOS=1`.
- Corrected `test/native/` path reference in `docs/troubleshooting.md`
  (correct path is `test/test_native/`).

### Changed
- Documentation improvements: Mermaid diagrams added to `timing-semantics.md`,
  `quick-start.md`, and `README.md`.
- Badges in `README.md` now include a note explaining they are pending registry
  indexing.

## [1.0.0] - 2026-05-09

### Added
- Initial public release.
- `taskscheduler::Task` and `taskscheduler::Scheduler` classes (also exposed as
  `TSTask` and `TSScheduler` global aliases).
- Background pump (`Scheduler::execute()`) — runs the single earliest-due
  task per call to avoid loop monopolisation.
- Critical pump (`Scheduler::executeCritical()`) — runs all due critical
  tasks per call.
- Optional `taskscheduler::FreeRTOSCriticalRunner` (ESP32) that drives the
  critical pump on a dedicated FreeRTOS task at a configurable tick.
- Pluggable `TimeProvider` for tests / non-Arduino hosts.
- Per-task statistics: runs, last/avg/max/total execution time,
  next-run time.
- `Scheduler::printStats(Print&)` that streams a one-line-per-task report
  to any Arduino `Print` (e.g. `Serial`).
- Examples: `01_BasicBlink`, `02_CriticalVsBackground`, `03_DelayedAndStats`.
- Documentation: quick start, API reference, timing semantics, troubleshooting.
- Native Unity tests under `test/`.

[1.0.2]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/andrenepomuceno/CriticalTaskScheduler/releases/tag/v1.0.0
