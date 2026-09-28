# Smart Medicine Reminder - System Integration Documentation

## 1. Overview

This document describes the complete integration between:
1. **Role 1 (Medicine & Cell Data)**: Dynamic JSON configuration loader (`configuration.json`).
2. **Role 2 (Scheduling & Time/Event Engine)**: Real-time RTC-driven scheduling, simultaneous due dose detection, and grace-period missed dose evaluation.
3. **Role 3 (OLED UI & Notification Engine)**: Non-blocking visual renderer for clock, active due alerts, missed banners, and dose confirmations.
4. **Role 4 (4-Cell Hardware & Alert Control)**: Independent RGB LEDs, dedicated servos, debounced buttons, and arbitrated common alert buzzer.

---

## 2. Architecture & Event Flow

                           +-------------------------------------+
                           | medicine-cell-data/config/          |
                           | configuration.json (Flash Embedded) |
                           +-------------------------------------+
                                              |
                                              v (loadConfiguration)
                           +-------------------------------------+
                           | RuntimeConfiguration (In-Memory)    |
                           +-------------------------------------+
                                              |
                                              v (ConfigurationAdapter::loadIntoEngine)
+-----------------------+  Tick / Now()       v
| DS1307 Real-Time Clock| -----------> +-------------------------------------+
| (RTCTimeProvider)     |               | ScheduleEventEngine                 |
+-----------------------+               | - Multi-cell schedule evaluation    |
                                        | - Simultaneous dose detection       |
                                        | - Grace period (Due -> Missed)      |
                                        | - Low-stock check on every tick     |
                                        +-------------------------------------+
                                              |
                                              | Emits DoseEvent to all listeners
                   +--------------------------+---------------------------+
                   | (DOSE_DUE, DOSE_MISSED,                              |
                   |  DOSE_CONFIRMED, DOSE_DISMISSED, LOW_STOCK_WARNING)  |
                   v                                                      v
+------------------------------------+               +------------------------------------+
| OledNotificationEngine (Role 3)    |               | HardwareBridge -> CellHardware (4) |
| - Clock Screen (Time & Next Dose)  |               | - Cell 1: Red GPIO 25, Grn 26,     |
| - Medicine Due Screen (Take Med)   |               |           Btn 32, Servo GPIO 18    |
| - Missed Dose Screen (Warning)     |               | - Cell 2: Red GPIO 14, Grn 16,     |
| - Dose Confirmed Screen (Taken)    |               |           Btn 33, Servo GPIO 19    |
| - Reminder Cleared (Dismissed)     |               | - Cell 3: Red GPIO 13, Grn 12,     |
| - Low Stock Warning Screen         |               |           Btn 27, Servo GPIO 5     |
| - Multi-notification auto-rotation |               | - Cell 4: Red GPIO 4,  Grn 2,      |
+------------------------------------+               |           Btn 23, Servo GPIO 17    |
                                                     | - Common Buzzer: GPIO 15 (Tone)    |
                                                     +------------------------------------+
                                                                         |
                                                                         | Physical Button Press
                                                                         v
                                                     +------------------------------------+
                                                     | onHardwareDoseConfirmed(cellId)    |
                                                     | -> engine.handleCellButtonPress()  |
                                                     | -> Decrements engine cell stock    |
                                                     +------------------------------------+
```

---

## 3. Dynamic Configuration Loading

Instead of hard-coding medicines, cell compartments, and schedule hours in code, all schedule metadata is loaded directly from `medicine-cell-data/config/configuration.json`:

```json
{
    "cellCount": 4,
    "cells": [
        { "cellId": "CELL01", "state": "OCCUPIED", "currentStock": 20, "medicine": { "name": "Paracetamol", "schedules": [{ "hour": 8, "minute": 0 }, { "hour": 20, "minute": 0 }] } },
        { "cellId": "CELL02", "state": "OCCUPIED", "currentStock": 10, "medicine": { "name": "Cetirizine",  "schedules": [{ "hour": 22, "minute": 0 }] } },
        { "cellId": "CELL03", "state": "OCCUPIED", "currentStock": 15, "medicine": { "name": "Amoxicillin", "schedules": [{ "hour": 13, "minute": 0 }, { "hour": 22, "minute": 0 }] } },
        { "cellId": "CELL04", "state": "OCCUPIED", "currentStock": 8,  "medicine": { "name": "Pantocid",    "schedules": [{ "hour": 7, "minute": 0 }] } }
    ]
}
```

- Embedded directly into the firmware image via PlatformIO `board_build.embed_txtfiles`.
- Parsed at runtime using `loadConfiguration(json, runtimeConfig)`.
- Injected into the scheduling engine via `ConfigurationAdapter::loadIntoEngine(runtimeConfig, scheduleEngine)`.

---

## 4. Hardware and Event Mapping

| Event Type | Scheduling Trigger | 4-Cell Hardware Behavior | OLED UI Display |
| :--- | :--- | :--- | :--- |
| **`DOSE_DUE`** | RTC time matches scheduled hour & minute | - Correct cell RGB LED → **RED**<br>- Correct cell Servo **OPENS to 90°**<br>- Common Buzzer **pulses (300ms on/off)** | Title: `MEDICINE DUE`<br>Shows medicine name, Cell ID, scheduled time, `TAKE MEDICINE`<br>Rotates every 1.8 s if multiple doses are due |
| **`DOSE_MISSED`** | Grace period (default 5 min) expires without confirmation | - Correct cell RGB LED → **YELLOW** (Warning)<br>- Correct cell Servo **CLOSES to 0°**<br>- Buzzer **stops** for this cell | Title: `MISSED DOSE`<br>Shows medicine name, Cell ID, scheduled time, `PRESS CONFIRM` |
| **`DOSE_CONFIRMED`** | Patient presses physical debounced button while dose is DUE | - Cell RGB LED → **GREEN for 1 s**<br>- Cell Servo **CLOSES to 0°**<br>- Common Buzzer re-arbitrated<br>- **Engine cell stock decremented by 1** | Title: `DOSE CONFIRMED`<br>Shows medicine name, Cell ID, `DOSE 1 TAKEN`<br>Held for 3 s then returns to next alert or clock |
| **`DOSE_DISMISSED`** | Patient presses button while dose is MISSED (missed reminder dismissed) | - Cell Yellow LED **OFF** (returns to IDLE) | Title: `REMINDER CLEARED`<br>Shows medicine name, Cell ID<br>Held for 3 s then returns to clock |
| **`LOW_STOCK_WARNING`** | `currentStock <= lowStockThreshold` detected on engine tick after a dose confirmation | - No hardware change (informational only) | Title: `LOW STOCK WARNING`<br>Shows medicine name, Cell ID, `STOCK: <n>`<br>Held for 3 s then returns to clock |
| **Idle State** | No due or missed doses active | - All LEDs OFF<br>- All Servos CLOSED (0°)<br>- Buzzer SILENT | Title: `SMART MEDICINE`<br>Header: `CURRENT TIME`<br>Digital Clock: `HH:MM:SS`<br>Footer: `NEXT HH:MM:SS <MedicineName>` |

---

## 5. Simultaneous Event Handling

When multiple medicines have the same scheduled timing (such as **Cell 2 Cetirizine** and **Cell 3 Amoxicillin** at 22:00):
1. **Hardware Independence**: Both Servo 2 and Servo 3 rotate to 90°. Both LED 2 and LED 3 turn RED. Servos 1 and 4 remain closed.
2. **Common Buzzer Arbitration**: The buzzer alert count becomes 2. The buzzer pulses continuously.
3. **OLED Rotation**: The OLED notification engine automatically alternates between the notifications for Cell 2 and Cell 3 every 1.8 seconds.
4. **Individual Confirmation**:
   - If the patient confirms Cell 2, Servo 2 closes and LED 2 turns GREEN for 1s.
   - The buzzer **continues beeping** because Cell 3 is still awaiting confirmation!
   - When Cell 3 is confirmed, Servo 3 closes, LED 3 turns GREEN, and the buzzer **silences immediately**.

---

## 6. Testing Guide (Wokwi Simulator)

### Starting the Simulator
1. Open `diagram.json` in VS Code.
2. Press `F1` and select `Wokwi: Start Simulator` (or click Play).
3. The OLED will show the real-time clock and the next scheduled dose.

### Interactive Serial Commands
You can type these commands in the Serial Monitor (115200 baud) at any time:

| Command | Description | What to Expect |
| :--- | :--- | :--- |
| `STATUS` | Full diagnostic state dump | Shows state of all 4 cells, servo angles, buzzer status, active due events, and active missed events. |
| `TIME <HH> <MM> [SS]` | Adjusts RTC time | E.g. `TIME 07 59 55` puts the clock 5 s before Pantocid (07:00). After 5 s, Servo 4 opens, LED 4 turns RED, Buzzer pulses, OLED shows Pantocid alert. |
| `TIME 21 59 55` | Test simultaneous doses | Reaches 22:00:00. Servos 2 & 3 open simultaneously. OLED rotates between Cetirizine and Amoxicillin. |
| `CONFIRM <1-4>` | Simulates button press on a DUE cell | Confirms dose; servo closes, LED turns green, OLED shows `DOSE CONFIRMED`. Also **decrements engine stock** — trigger `LOWSTOCK` after enough confirms. |
| `DUE <1-4>` | Manually triggers DUE state | Directly activates medicine-due state for Cell X. |
| `MISSED <1-4>` | Manually triggers MISSED state | Puts Cell X into yellow warning state. |
| `RESET <1-4>` | Resets single cell | Resets Cell X to IDLE and dismisses its missed reminder (OLED shows `REMINDER CLEARED` for 3 s). |
| `RESET` | Global reset | Resets all cells and clears all pending reminders. |
| `LOWSTOCK <1-4>` | **Simulates low stock for Cell X** | Forces `currentStock` to 1 for Cell X. On the next engine tick, a `LOW_STOCK_WARNING` event fires. OLED shows `LOW STOCK WARNING` with medicine name and remaining count for 3 s. |
| `GRACE <minutes>` | Sets grace period | Adjusts grace period before due alerts transition to missed (e.g. `GRACE 1`). |

---

## 7. Modified and Added Files

- **`platformio.ini`**: Added include build flags (`-Ischeduling-event-engine/include`, `-Imedicine-cell-data/include`, `-Ilib/oled-ui-notification-engine/include`).
- **`src/HardwareBridge.h`**: Clean interface isolating Role 4 hardware functions to prevent type collisions between `CellState` enums.
- **`src/HardwareBridge.cpp`**: Hardware bridge implementation managing callbacks and `CellHardwareController`.
- **`src/main.cpp`**: Master coordinator binding RTC time provider, scheduler, dynamic configuration loader, OLED engine, and hardware bridge.
- **`docs/system_integration.md`**: Complete system documentation.
