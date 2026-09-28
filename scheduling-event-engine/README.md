# Scheduling & Time / Event Engine

## 1. Module Overview
The **Scheduling & Time/Event Engine** is the core real-time processing engine of the **Smart Medicine Reminder and Dose Management System**. It evaluates medication schedules against the current real-time clock (RTC), triggers audible and visual alarms when doses are due, detects simultaneous multi-cell schedules, monitors alert grace periods, transitions unconfirmed events to missed dose notifications (mobile clock reminder style), and dispatches structured event notifications to downstream subsystems.

```txt
       +---------------------------------------------+
       |         Configuration Snapshot (JSON)       |
       +---------------------------------------------+
                              │
                              ▼
       +---------------------------------------------+
       |   RuntimeConfiguration (medicine-cell-data)  |
       +---------------------------------------------+
                              │
                              ▼
       +---------------------------------------------+
       |             ConfigurationAdapter            |
       +---------------------------------------------+
                              │
                              ▼
  [ RTC Clock ] ──► +---------------------------------------------+
  (ITimeProvider)   |          ScheduleEventEngine                |
                    |  - Time Evaluation & Due Trigger           |
                    |  - Simultaneous Multi-Dose Queue            |
                    |  - Grace Period & Missed Dose Detection     |
                    |  - Mobile Clock App Missed Logic            |
                    |  - Midnight Rollover & Daily Reset          |
                    +---------------------------------------------+
                                       │
           ┌───────────────────────────┼───────────────────────────┐
           ▼                           ▼                           ▼
+---------------------+     +---------------------+     +---------------------+
|   OLED UI Engine    |     | Cell Hardware Alert |     |  Dose & Stock Mgr   |
| (Display & Rolling) |     |  (LEDs & Buzzers)   |     |  (Stock Decrement)  |
+---------------------+     +---------------------+     +---------------------+
```

---

## 2. Directory Structure
```txt
scheduling-event-engine/
├── docs/
│   ├── api_reference.md       # Full API documentation and teammate integration contracts
│   └── architecture.md        # State machine, sequence diagrams, and design decisions
├── include/
│   ├── configuration_adapter.hpp  # Adapter bridging RuntimeConfiguration with Engine
│   ├── schedule_event_engine.hpp  # Core engine class declaration
│   ├── time_engine_types.hpp      # Event types, EventState, TimeOfDay, DoseEvent
│   └── time_provider.hpp          # ITimeProvider, RTCTimeProvider, SimulatedTimeProvider
├── sample/
│   ├── README.md              # Instructions for running the sample simulation
│   └── sample_main.cpp        # Full end-to-end runnable demonstration
├── src/
│   ├── configuration_adapter.cpp  # Adapter implementation
│   ├── schedule_event_engine.cpp  # Engine core implementation
│   └── time_provider.cpp          # Hardware RTC & Simulated time implementations
├── test/
│   ├── README.md              # Test documentation and test case rationale
│   ├── run_tests.sh           # Native automated test runner script
│   └── test_engine_unit.cpp   # Comprehensive 11-scenario unit test suite
└── README.md                  # This documentation file
```

---

## 3. Core Responsibilities & Feature Highlights

### 3.1. Real-Time Schedule Evaluation
- Queries time from `ITimeProvider` (supporting physical RTC DS1307/DS3231 via RTClib or `SimulatedTimeProvider`).
- Matches current hour and minute with enabled schedules.
- Prevents redundant firing within the same minute.

### 3.2. Simultaneous Dose Handling
- When multiple medications are scheduled at the exact same timestamp (e.g. Paracetamol in Cell 1 and Amoxicillin in Cell 3 both at 08:00):
  - Both doses generate individual `DOSE_DUE` events concurrently.
  - Active due list tracks all simultaneous events.
  - The OLED UI rolls through medication names sequentially.
  - Cell LEDs blink or glow RED for each due compartment.
  - The shared alert buzzer keeps beeping until all due doses are confirmed or transition to missed.

### 3.3. Mobile Clock App Missed Dose Logic
- A configurable grace period (default 5–10 minutes) begins when a dose becomes due.
- If the patient does not press the cell confirm button before the grace period elapses:
  - The event transitions from `EventState::DUE` to `EventState::MISSED`.
  - The loud audible buzzer ceases.
  - A persistent missed dose notification banner is shown on the OLED.
  - When the patient presses CONFIRM on any cell to acknowledge the reminder:
    - The missed notification is dismissed (`EventType::DOSE_DISMISSED`).
    - **Crucially, medicine stock is NOT decremented**, maintaining accurate physical inventory counts.

### 3.4. Dose Confirmation & Stock Interaction
- When the patient presses the confirm button during the `DUE` window:
  - Event transitions to `EventState::CONFIRMED`.
  - Emits `EventType::DOSE_CONFIRMED`.
  - Downstream stock manager decrements the medicine count.
  - Hardware alert controller turns the cell LED GREEN for 1 second.

### 3.5. Midnight Rollover & Daily Reset
- Automatic detection of midnight boundary crossing (e.g. 23:59:59 to 00:00:00).
- Refreshes all `triggeredToday` flags for all cells.
- Emits `EventType::DAILY_RESET`.

### 3.6. Scalable & Embedded-Friendly Architecture
- Zero dynamic memory allocations (`new`/`malloc`) inside real-time loops.
- Bounded static memory footprint.
- Tested across 1 cell, 4 cells (2x2), 16 cells (4x4), and extensible to 64 cells (8x8) via `-DMAX_ENGINE_CELLS=64`.

---

## 4. Verification & Testing

### 4.1. Native Unit Tests (11 Scenarios)
Execute the native automated test suite:
```bash
./scheduling-event-engine/test/run_tests.sh
```
Output:
```txt
==========================================
 Building Scheduling & Time/Event Engine Unit Tests
==========================================
Executing test suite...

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

### 4.2. Sample Demonstration
Compile and run the interactive demo:
```bash
clang++ -std=c++14 -Wall -Wextra \
    -I scheduling-event-engine/include \
    scheduling-event-engine/src/schedule_event_engine.cpp \
    scheduling-event-engine/src/time_provider.cpp \
    scheduling-event-engine/src/configuration_adapter.cpp \
    scheduling-event-engine/sample/sample_main.cpp \
    -o scheduling-event-engine/sample/sample_demo

./scheduling-event-engine/sample/sample_demo
```

### 4.3. PlatformIO Firmware Build
Build target firmware for ESP32:
```bash
/Users/gabriel/.platformio/penv/bin/pio run
```
Output:
```txt
Building in release mode
Compiling .pio/build/esp32doit-devkit-v1/scheduling-event-engine/time_provider.o
Linking .pio/build/esp32doit-devkit-v1/firmware.elf
Retrieving maximum program size .pio/build/esp32doit-devkit-v1/firmware.elf
Checking size .pio/build/esp32doit-devkit-v1/firmware.elf
RAM:   [=         ]   6.7% (used 21952 bytes from 327680 bytes)
Flash: [==        ]  23.1% (used 303113 bytes from 1310720 bytes)
========================= [SUCCESS] =========================
```

---

## 5. Downstream Modules Integration Summary

| Subsystem | Triggering Event | Action Required |
|---|---|---|
| **OLED UI Engine (Task 3)** | `DOSE_DUE` | Display medicine name, cell ID, quantity. Roll through names if simultaneous. |
| | `DOSE_MISSED` | Display persistent missed reminder banner until confirmed. |
| | `DOSE_DISMISSED` | Clear missed banner. |
| **Alert Hardware (Task 4)** | `DOSE_DUE` | Cell RGB LED -> RED. Alert Buzzer -> BEEPING. |
| | `DOSE_CONFIRMED` | Cell RGB LED -> GREEN (1 sec). Alert Buzzer -> OFF. |
| | `DOSE_MISSED` | Alert Buzzer -> OFF (silent reminder mode). |
| | `LOW_STOCK_WARNING` | Cell RGB LED -> YELLOW. Warning Buzzer -> Chirp. |
| **Stock Management (Task 5)** | `DOSE_CONFIRMED` | Decrement cell inventory by `doseQuantity`. |
| | `DOSE_DISMISSED` | Do NOT decrement inventory. |
