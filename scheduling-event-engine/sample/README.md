# Scheduling & Time/Event Engine - Sample Demonstration

## Overview
This directory contains sample demonstrations illustrating how the **Scheduling & Time/Event Engine** integrates with simulated time, triggers events, and handles patient interactions (such as dose confirmation, missed reminders, and midnight rollovers).

---

## Contents
- `sample_main.cpp`: Complete executable example modeling:
  - 4 cells (2x2 modular matrix)
  - Simultaneous dose scheduling (Paracetamol and Amoxicillin at 08:00)
  - Downstream event listeners (OLED UI display, Hardware Buzzer & RGB LED alerts, Stock Manager)
  - Time fast-forwarding to trigger doses, confirm doses, and transition unconfirmed doses to missed
  - Patient pressing confirm to dismiss missed reminders without decrementing stock (mobile clock app style)
  - Midnight rollover daily reset

---

## How to Compile and Run the Sample

Compile natively using `clang++` or `g++`:
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

Clean up binary afterwards:
```bash
rm -f scheduling-event-engine/sample/sample_demo
```

---

## Expected Demonstration Output

```txt
========================================================
 SMART MEDICINE REMINDER - SCHEDULING ENGINE DEMO
========================================================

Engine initialized with 4 cells.
Current Simulated Time: 07:59:55
Active Due Doses: 0

--- Advancing time by 5 seconds to 08:00:00 ---

>>> [EVENT RECEIVED] EVENT_DOSE_DUE
    [OLED UI] Displaying Reminder: Paracetamol (Cell: CELL01, Qty: 1)
    [ALERT HARDWARE] Cell CELL01 LED -> RED
    [ALERT HARDWARE] Alert Buzzer -> BEEPING...

>>> [EVENT RECEIVED] EVENT_DOSE_DUE
    [OLED UI] Displaying Reminder: Amoxicillin (Cell: CELL03, Qty: 1)
    [ALERT HARDWARE] Cell CELL03 LED -> RED
    [ALERT HARDWARE] Alert Buzzer -> BEEPING...

Active Due Doses now: 2
Multiple medicines due at once: YES

--- Patient presses CONFIRM button on CELL01 ---

>>> [EVENT RECEIVED] EVENT_DOSE_CONFIRMED
    [ALERT HARDWARE] Cell CELL01 LED -> GREEN (for 1 sec)
    [ALERT HARDWARE] Alert Buzzer -> OFF
    [STOCK MANAGER] Confirmed! Decrementing stock for Paracetamol
Remaining Due Doses: 1

--- Advancing time by 6 minutes to 08:06:00 (Grace period was 5 min) ---

>>> [EVENT RECEIVED] EVENT_DOSE_MISSED (Clock App Reminder Style)
    [OLED UI] Persistent Banner: MISSED DOSE - Amoxicillin scheduled for 08:00
    [ALERT HARDWARE] Audible Buzzer stopped. Reminder pending user action.

Active Due Doses: 0
Active Missed Reminders: 1

--- Patient presses CONFIRM on any cell to clear missed notification ---

>>> [EVENT RECEIVED] EVENT_DOSE_DISMISSED
    [OLED UI] Cleared missed dose banner for Amoxicillin
    [STOCK MANAGER] Note: Stock is NOT decremented for missed dose dismissal.
Active Missed Reminders after dismissal: 0

--- Advancing time to Midnight (23:59:50 -> 00:00:00) ---

>>> [EVENT RECEIVED] EVENT_DAILY_RESET
    [ENGINE] Midnight rollover complete. All daily schedules refreshed for today.

========================================================
 DEMO COMPLETED SUCCESSFULLY!
========================================================
```
