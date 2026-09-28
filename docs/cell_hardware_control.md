# Role 4: Cell Hardware & Alert Control - Technical Documentation (4-Cell Architecture)

## 1. Module Overview & Responsibilities

In the **Smart Medicine Reminder and Dose Management System**, the system architecture strictly separates **scheduling logic** from **physical hardware actuation**.

### Role Scope & Boundary Contract
- **Our Responsibility (Role 4):** The physical hardware control of **exactly 4 medicine compartments**, alerts, actuators, debouncing, and local state transitions.
- **Architectural Principle:**
  - The Scheduling Engine (Role 2) decides: *"When is a medicine due?"*
  - The Dose Manager (Role 5) decides: *"When to decrement inventory and log dose history."*
  - Our Module (Role 4) decides: *"What does the physical hardware (LED, Buzzer, Servo) do when an event occurs, and when the user physically presses the confirm button?"*

```
   [Role 2: Scheduler]                       [Role 5: Dose Manager]
           │                                            ▲
           │ EVENT_MEDICINE_DUE(cell_id)                │ DOSE_CONFIRMED(cell_id)
           │ EVENT_MISSED_DOSE(cell_id)                 │ (Decrements inventory)
           ▼                                            │
┌─────────────────────────────────────────────────────────────────────────┐
│              ROLE 4: CELL HARDWARE & ALERT CONTROL                      │
│                                                                         │
│  ┌──────────────────────┐  ┌──────────────────────┐  ┌───────────────┐ │
│  │   Cell 1 Hardware    │  │   Cell 2 Hardware    │  │   Cell 3 & 4  │ │
│  │   • RGB: D25 / D26   │  │   • RGB: D14 / D16   │  │   • RGB: D13/12│ │
│  │   • Btn: D32         │  │   • Btn: D33         │  │     & D4/2    │ │
│  │   • Servo 1: D18     │  │   • Servo 2: D19     │  │   • Btn: D27/23│ │
│  │   • FSM (Indep.)     │  │   • FSM (Indep.)     │  │   • Servo 3 & 4│ │
│  └──────────────────────┘  └──────────────────────┘  └───────────────┘ │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │ Common Buzzer Arbitration (GPIO 15, LEDC Channel 6 / Timer 3)      │  │
│  │ • 2500 Hz intermittent pulse while active alerting count > 0       │  │
│  │ • Confirming Cell 1 does NOT stop alert if Cell 2 is still active! │  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Root Cause Analysis & Fixes: Cell 4 Servo & Common Buzzer

### Problem 1: Cell 4 Servo Not Responding (Cells 1–3 Working)
- **Root Cause Analysis:**
  1. **Wokwi Board Pin Name Mismatch:** In `HardwareConfig.h`, Servo 4 was assigned to GPIO 17 (`PIN_CELL4_SERVO = 17`). In `diagram.json`, it was wired to `esp:D17`. On the `wokwi-esp32-devkit-v1` part, GPIO 17 is the hardware UART2 TX pin and is silkscreened/keyed as **`TX2`**, NOT `D17`. Because no pin `D17` exists on the Wokwi DevKit V1 component, Wokwi dropped the connection, leaving Servo 4 floating/disconnected. (Similarly, `rgb2:G` was wired to `esp:D16` instead of `esp:RX2`).
  2. **Timer Collision on Timer 0:** In `ServoController.cpp`, `ESP32PWM::allocateTimer(0)` was allocating hardware Timer 0. Because the ESP32 Arduino framework's `tone()` implementation in `Tone.cpp` uses LEDC Channel 0 on Timer 0, allocating Timer 0 to `ESP32Servo` at 50 Hz created a timer collision.
- **Fixes Applied:**
  1. **Wokwi Connection Fix:** Updated `diagram.json` line 174 to connect `servo4:PWM` to `"esp:TX2"`. Updated line 151 to connect `rgb2:G` to `"esp:RX2"`.
  2. **Dedicated Servo Timer Allocation:** In `ServoController.cpp`, changed timer allocation to `ESP32PWM::allocateTimer(1); ESP32PWM::allocateTimer(2); ESP32PWM::allocateTimer(3);`. This leaves Timer 0 exclusively for `tone()`, while Timers 1, 2, and 3 provide 12 channels for the 4 50Hz servos with zero conflicts.

### Problem 2: Common Buzzer Completely Inactive
- **Root Cause Analysis:**
  1. **Wokwi Audio Simulation Trigger:** In Wokwi, the `wokwi-buzzer` component simulates an acoustic piezo element using browser Web Audio API that hooks directly into Arduino's standard `tone(pin, freq)` and `noTone(pin)` calls. Direct ESP-IDF `ledcSetup` / `ledcWrite` register calls bypass Wokwi's audio synthesis engine.
  2. **Timer Overwrite by ESP32Servo:** When `ESP32Servo` previously claimed Timer 0, any subsequent call to `tone()` failed or reconfigured the servo's PWM frequency.
  3. **Component Configuration:** Earlier `diagram.json` had `"hasSound": "true"` which is invalid in Wokwi, corrected to `"volume": "1.0"`.
- **Fixes Applied:**
  1. **Standard ESP32 Tone Generation:** In `BuzzerController.cpp`, implemented tone generation via `tone(_pin, _frequencyHz)` and `noTone(_pin)`.
  2. **State-Change Gating:** Added `_isToneActive` latching guard so `tone()` is only invoked on genuine rising/falling edges of the alert cadence, avoiding FreeRTOS queue flooding in the main loop.
  3. **Multi-Cell Alert Arbitration:** Managed a central `_activeAlertingCount`. The buzzer turns on when the count rises from 0 to $\ge 1$, maintains cadence while count $> 0$, and silences only when count drops to 0.

### Test Verification & Results
- Verified with embedded PlatformIO builds and standalone native unit tests (`test_cell_hardware_native.cpp`):
  - **Test 1:** Initial 4-cell state (all 4 cells IDLE, all 4 servos closed at 0°, buzzer silent) -> **PASSED**.
  - **Test 2:** Cell 4 servo independently opens to 90° on `MEDICINE_DUE` while Servos 1, 2, 3 remain closed at 0° -> **PASSED**.
  - **Test 3:** Cells 1, 2, 3 servos independently open to 90° on their respective alerts -> **PASSED**.
  - **Test 4:** Common buzzer arbitration with simultaneous alerts (Cells 1 and 2 due -> confirm Cell 1 -> buzzer remains ON because Cell 2 is still active -> confirm Cell 2 -> buzzer silences immediately) -> **PASSED**.
  - **Test 5:** Physical button debounce (50ms) and 1-second green LED confirmation auto-reset -> **PASSED**.

---

## 3. Hardware Architecture & Complete Pin Allocation

All 4 cells, actuators, and existing peripherals operate without pin or timer conflicts:

| Subsystem | Component | ESP32 GPIO (Pin / Silkscreen) | Mode | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **Existing I2C** | OLED & RTC SDA | `GPIO 21` (`esp:D21`) | Open-Drain | I2C Data bus |
| **Existing I2C** | OLED & RTC SCL | `GPIO 22` (`esp:D22`) | Open-Drain | I2C Clock bus |
| **Existing UART**| TX0 / RX0 | `GPIO 1 / 3` (`esp:TX0`, `esp:RX0`) | Serial | USB Serial Monitor at 115200 baud |
| **Common Buzzer**| Piezo Buzzer | `GPIO 15` (`esp:D15`) | `OUTPUT` | Common alert tone (Timer 0, 2500 Hz `tone()`) |
| **Cell 1** | RGB Red | `GPIO 25` (`esp:D25`) | `OUTPUT` | Active-HIGH anode |
| **Cell 1** | RGB Green | `GPIO 26` (`esp:D26`) | `OUTPUT` | Active-HIGH anode |
| **Cell 1** | Push Button | `GPIO 32` (`esp:D32`) | `INPUT_PULLUP` | Active-LOW confirm button |
| **Cell 1** | Compartment Servo 1 | `GPIO 18` (`esp:D18`) | `OUTPUT` | Dedicated 50 Hz PWM servo |
| **Cell 2** | RGB Red | `GPIO 14` (`esp:D14`) | `OUTPUT` | Active-HIGH anode |
| **Cell 2** | RGB Green | `GPIO 16` (`esp:RX2`) | `OUTPUT` | Active-HIGH anode |
| **Cell 2** | Push Button | `GPIO 33` (`esp:D33`) | `INPUT_PULLUP` | Active-LOW confirm button |
| **Cell 2** | Compartment Servo 2 | `GPIO 19` (`esp:D19`) | `OUTPUT` | Dedicated 50 Hz PWM servo |
| **Cell 3** | RGB Red | `GPIO 13` (`esp:D13`) | `OUTPUT` | Active-HIGH anode |
| **Cell 3** | RGB Green | `GPIO 12` (`esp:D12`) | `OUTPUT` | Active-HIGH anode (safe boot low) |
| **Cell 3** | Push Button | `GPIO 27` (`esp:D27`) | `INPUT_PULLUP` | Active-LOW confirm button |
| **Cell 3** | Compartment Servo 3 | `GPIO 5`  (`esp:D5`)  | `OUTPUT` | Dedicated 50 Hz PWM servo |
| **Cell 4** | RGB Red | `GPIO 4`  (`esp:D4`)  | `OUTPUT` | Active-HIGH anode |
| **Cell 4** | RGB Green | `GPIO 2`  (`esp:D2`)  | `OUTPUT` | Active-HIGH anode (safe boot low) |
| **Cell 4** | Push Button | `GPIO 23` (`esp:D23`) | `INPUT_PULLUP` | Active-LOW confirm button |
| **Cell 4** | Compartment Servo 4 | `GPIO 17` (`esp:TX2`) | `OUTPUT` | Dedicated 50 Hz PWM servo |

---

## 4. Per-Cell State Machine

Each of the 4 cells independently executes this FSM:

```
               ┌──────────────┐
               │     IDLE     │◄──────────────────┐
               └──────┬───────┘                   │
                      │                           │
  EVENT_MEDICINE_DUE  │                           │ Auto-timeout
  (for this cell)     ▼                           │ (after 1000ms)
               ┌──────────────┐                   │
    ┌─────────►│   ALERTING   │                   │
    │          │  (RGB: RED)  │                   │
    │          │ (Servo: OPEN)│                   │
    │          └──────┬───────┘                   │
    │                 │                           │
    │  EVENT_         │ Push Button               │
    │  MISSED_DOSE    │ Pressed (this cell)       │
    │                 ▼                           │
    │          ┌──────────────┐                   │
    │          │  CONFIRMED   ├───────────────────┘
    │          │ (RGB: GREEN) │
    │          │(Servo: CLOSE)│
    │          └──────────────┘
    │                 ▲
    │ Push Button     │
    │ Pressed         │
    ▼                 │
┌──────────────┐      │
│   WARNING    ├──────┘
│(RGB: YELLOW) │
│(Servo: CLOSE)│
└──────────────┘
```

---

## 5. Multi-Cell Arbitration & Simultaneous Events

- **Independent Actuation:** When Cell 1 becomes due, **only** Servo 1 opens (rotates to 90°) and **only** Cell 1's RGB LED turns RED. Servos 2, 3, and 4 remain closed (0°).
- **Common Buzzer Logic:**
  - Active alerting count = $\sum_{i=0}^{3} (\text{State}_i == \text{ALERTING} ? 1 : 0)$.
  - If count $> 0$: Common buzzer pulses continuously in 300ms ON / 300ms OFF intervals.
  - If Cell 1 is confirmed while Cell 2 is still ALERTING:
    - Cell 1 transitions to CONFIRMED (Green for 1s), Servo 1 closes to 0°.
    - Cell 2 remains RED and Servo 2 remains OPEN.
    - Active alert count decreases from 2 to 1.
    - **The common buzzer continues beeping because Cell 2 is still active!**
    - Only when Cell 2 is also confirmed (count drops to 0) does the common buzzer stop immediately.

---

## 6. Automated Unit Tests

Automated tests in `test/test_cell_hardware/test_cell_hardware.cpp` validate:
1. `test_initial_4cell_state`: All 4 cells start IDLE, all 4 servos closed, buzzer silent.
2. `test_individual_servos_open_only_for_due_cell`: Triggering DUE on Cell 1 opens Servo 1 (90°) while Servos 2, 3, 4 remain strictly at 0°.
3. `test_confirm_closes_only_that_cells_servo`: Confirming Cell 1 closes Servo 1 while Cell 2's servo remains open at 90°.
4. `test_common_buzzer_arbitration_across_all_4_cells`: Simultaneous alert across all 4 cells maintains buzzer until the last cell is confirmed.
5. `test_missed_dose_warning_per_cell`: Missed dose sets LED to Yellow and closes servo.
6. `test_reset_functionality`: Resets cell cleanly to IDLE.

All unit tests compiled with **exit code 0**.
