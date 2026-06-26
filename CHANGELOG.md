# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.5] - 2026-06-26

### Fixed
- **CI: native unit tests on 64-bit hosts.** The `millis()` rollover tests
  hardcoded a 32-bit wrap (`0xFFFFFFFF`), which only holds where
  `unsigned long` is 32-bit (AVR/ARM). On the 64-bit native CI runner the
  simulated wrap never happened and the tests failed. They now anchor to
  `(unsigned long)-1`, so the rollover is exercised at the host's real
  `unsigned long` width. Library behaviour on real MCUs is unchanged.
- **CI: arduino-lint.** Switched `library-manager: submit` to `update`; the
  library is already in the Library Manager index, where `submit` errors with
  rule LP017 ("name in use").
- **Release workflow: GitHub Release step.** Release notes are now passed via
  an environment variable and `--notes-file` instead of being interpolated
  inline into the shell. Backticks/parentheses in the changelog were being
  evaluated by the shell, which broke the step and skipped the dependent
  PlatformIO publish.

## [1.0.4] - 2026-06-26

### Changed
- **Discoverability.** Reworded `library.properties` `sentence` and
  `library.json` `description` to lead with the exact phrase "Task scheduler"
  so the library ranks for generic `task scheduler` searches in the Arduino
  Library Manager and PlatformIO Registry (not only the exact library name).

### Fixed
- **Broken links in the registry article.** All relative links in `README.md`
  (other-language READMEs, `docs/`, `examples/`, `LICENSE`) are now absolute
  GitHub URLs. The PlatformIO and Arduino registries render `README.md`
  standalone and do not resolve relative paths, so those links were broken on
  the published pages.

## [1.0.3] - 2026-05-09

### Fixed
- **`millis()` rollover bug.** `Task::shouldRun()` and the earliest-due
  selection in `Scheduler::execute()` used a naive `>=` comparison on
  `unsigned long`, which broke around the ~49.7-day `millis()` wrap (tasks
  could fire instantly or stop firing for ~49 days). Both now use
  rollover-safe arithmetic (`(long)(now - nextRunTime) >= 0` and
  lateness-based selection).
- **Duplicate task registration.** `Scheduler::addTask()` now rejects a
  `Task*` already registered in the same bucket (returns `false`).
  Previously a duplicate would double-fire its callback per pump and corrupt
  its stats.
- **`FreeRTOSCriticalRunner` tick-rate edge case.** `pdMS_TO_TICKS(tickMs)`
  is now clamped to `>= 1`, preventing undefined behaviour from
  `vTaskDelayUntil` on FreeRTOS configurations where the tick rate would
  round the requested period down to zero.

### Changed
- Documented thread-safety contract on `FreeRTOSCriticalRunner`: configure
  all critical tasks before `start()`; do not mutate the critical bucket
  while the runner is running.

### Added
- Unit tests covering `millis()` rollover (`shouldRun` + `execute`) and
  duplicate-rejection in `addTask` (4 new tests, all passing on native).

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

[1.0.5]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/andrenepomuceno/CriticalTaskScheduler/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/andrenepomuceno/CriticalTaskScheduler/releases/tag/v1.0.0
