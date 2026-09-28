# Scheduling & Time/Event Engine - Test Suite Documentation

## Overview
This directory contains the automated unit test suite for the **Scheduling & Time/Event Engine** module. The tests validate all real-time scheduling logic, event generation, boundary conditions, edge cases, and hardware abstraction.

The test suite runs natively on macOS/Linux using `clang++` or `g++`, executing without requiring physical ESP32 or RTC hardware via the `SimulatedTimeProvider`.

---

## How to Run the Tests

Execute the automated test runner script:
```bash
./scheduling-event-engine/test/run_tests.sh
```

Or manually compile and run using `clang++`:
```bash
clang++ -std=c++14 -Wall -Wextra \
    -I scheduling-event-engine/include \
    scheduling-event-engine/src/schedule_event_engine.cpp \
    scheduling-event-engine/src/time_provider.cpp \
    scheduling-event-engine/src/configuration_adapter.cpp \
    scheduling-event-engine/test/test_engine_unit.cpp \
    -o scheduling-event-engine/test/test_runner

./scheduling-event-engine/test/test_runner
rm -f scheduling-event-engine/test/test_runner
```

---

## Test Cases Detailed Breakdown

| # | Test Name | Objective & Scenario | Validation Criteria |
|---|---|---|---|
| **1** | `test_single_schedule_trigger` | Simulates a single cell with Paracetamol scheduled at 08:00. Time advances from 07:59:50 to 08:00:00. | Verifies no false trigger at 07:59:50. At 08:00:00, emits `EventType::DOSE_DUE`, sets `state = EventState::DUE`, increments active due event count to 1. |
| **2** | `test_simultaneous_schedules` | Two cells (Cell 0: Paracetamol, Cell 1: Amoxicillin) are both scheduled at 08:00. Cell 2 is scheduled at 22:00. | When clock reaches 08:00, both Cell 0 and Cell 1 trigger `DOSE_DUE` simultaneously. Active due count is 2. Cell 2 does not trigger. |
| **3** | `test_dose_confirmation` | Cell 0 is in `DUE` state. Patient presses the confirmation button for Cell 0. | `confirmDose(0)` succeeds. Emits `EventType::DOSE_CONFIRMED`, removes event from active due list, sets state to `CONFIRMED`. |
| **4** | `test_grace_period_missed_dose` | Grace period set to 10 minutes. Dose scheduled at 08:00. Time advances to 08:05 (within grace period), then to 08:11 (grace period expired). | At 08:05, event remains `DUE`. At 08:11, transitions to `EventState::MISSED`, moves to active missed list, emits `EventType::DOSE_MISSED`. |
| **5** | `test_dismiss_missed_dose_no_stock_decrement` | Cell is in `MISSED` state. Patient presses confirm button on that cell to dismiss the reminder (mobile clock app logic). | `handleCellButtonPress(0)` dismisses the missed reminder banner, emits `DOSE_DISMISSED`. Current stock remains untouched (no stock decrement). |
| **6** | `test_dismiss_all_missed_on_any_button` | Two cells have missed doses pending. Patient presses confirm button on any cell. | `dismissAllMissedDoses()` clears all pending missed reminders simultaneously. |
| **7** | `test_prevent_duplicate_triggers` | Engine `update()` is called multiple times within the same minute (08:00:10, 08:00:20, 08:00:50). | Dose triggers exactly once at 08:00:10; subsequent calls within the same minute do not produce duplicate events. |
| **8** | `test_midnight_rollover` | Time rolls over from 23:59:50 to 00:00:10 next day. Schedules that fired yesterday had `triggeredToday = true`. | On midnight boundary crossing, resets `triggeredToday` flags for all cells to `false` and emits `EventType::DAILY_RESET`. |
| **9** | `test_scalability_16_cells` | Instantiates a 16-cell system (4x4 matrix) with all 16 cells scheduled for 12:00. | All 16 cells are added, evaluated, and triggered cleanly without memory faults or stack overflow. |
| **10** | `test_configuration_adapter` | Creates a mock `RuntimeConfiguration` (teammate 1 data format) and loads it into the engine via `ConfigurationAdapter::loadIntoEngine()`. | All 4 cells, medicines, doses, and schedules are accurately converted and loaded into engine memory. |
| **11** | `test_next_scheduled_dose` | Schedules doses at 08:00 and 14:30. Current time is 10:00. | Engine correctly determines that the next upcoming dose is 14:30. |

---

## Test Execution Results

```txt
========================================
 SCHEDULING & TIME/EVENT ENGINE TESTS
========================================
[RUN] test_single_schedule_trigger...
  [PASS] Single schedule triggered at 08:00:00
[RUN] test_simultaneous_schedules...
  [PASS] Multiple simultaneous medicines detected at 08:00 (Count: 2)
[RUN] test_dose_confirmation...
  [PASS] Dose confirmed successfully during due window
[RUN] test_grace_period_missed_dose...
  [PASS] Grace period expired; transitioned to MISSED state
[RUN] test_dismiss_missed_dose_no_stock_decrement...
  [PASS] Missed dose dismissed properly; stock preserved at 20
[RUN] test_dismiss_all_missed_on_any_button...
  [PASS] All missed alerts cleared simultaneously
[RUN] test_prevent_duplicate_triggers...
  [PASS] No duplicate triggers inside same scheduled minute
[RUN] test_midnight_rollover...
  [PASS] Midnight rollover reset triggeredToday flags and emitted DAILY_RESET
[RUN] test_scalability_16_cells...
  [PASS] Scalability verified: 16 simultaneous cells handled cleanly
[RUN] test_configuration_adapter...
  [PASS] Configuration adapter successfully loaded 4 cells into engine
[RUN] test_next_scheduled_dose...
  [PASS] Next scheduled dose correctly identified as 14:30
========================================
 ALL 11 UNIT TESTS PASSED SUCCESSFULLY!
========================================
```
