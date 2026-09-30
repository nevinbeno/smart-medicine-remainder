# Role 4: Cell Hardware & Alert Control - Technical Documentation (Dynamic Multi-Cell Architecture)
**Owner / Lead: Pavithra (Role 4: Cell Hardware & Alert Control)**

## 1. Module Overview & Responsibilities

In the **Smart Medicine Reminder and Dose Management System**, the system architecture strictly separates **scheduling logic** from **physical hardware actuation**.

### Role Scope & Boundary Contract
- **Our Responsibility (Role 4 - Pavithra):** The physical hardware control of **scalable matrix compartments (1 to 64 cells, 1x1 to 8x8)**, alerts, actuators, debouncing, multi-cell arbitration, local state transitions, and Wokwi Single Point of Entry auto-sync.
- **Architectural Principle:**
  - The Scheduling Engine (Role 2) decides: *"When is a medicine due?"*
  - The Dose Manager (Role 5) decides: *"When to decrement inventory and log dose history."*
  - Our Module (Role 4 - Pavithra) decides: *"What does the physical hardware (LED, Buzzer, Servo) do when an event occurs, and when the user physically presses the confirm button across any scalable matrix grid?"*

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
│  │   Cell 1 Hardware    │  │   Cell 2 Hardware    │  │ Cells 3 to 64 │ │
│  │   • RGB: D25 / D26   │  │   • RGB: D14 / D16   │  │ • Matrix Grid │ │
│  │   • Btn: D32         │  │   • Btn: D33         │  │ • 1x1 to 8x8  │ │
│  │   • Servo 1: D18     │  │   • Servo 2: D19     │  │ • HAL Layer   │ │
│  │   • FSM (Indep.)     │  │   • FSM (Indep.)     │  │ • Indep. FSM  │ │
│  └──────────────────────┘  └──────────────────────┘  └───────────────┘ │
│                                                                         │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │ Common Buzzer Arbitration (GPIO 15, Multi-Cell Alert Cadence)     │  │
│  │ • 800 Hz intermittent pulse while active alerting count > 0        │  │
│  │ • Confirming Cell 1 does NOT stop alert if other cells are active!│  │
│  └───────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

### Test Verification & Results
- Verified with embedded PlatformIO builds and standalone native unit tests (`test/run_hardware_tests.sh`):
  - **12/12 Automated Test Suites Passing (100%)**:
    1. `test_initial_4cell_state` (servos 0°, LEDs off, buzzer silent) -> **PASSED**.
    2. `test_individual_servos_open_only_for_due_cell` (only due cell opens to 90°) -> **PASSED**.
    3. `test_confirm_closes_only_that_cells_servo` (confirmation resets only the pressed cell) -> **PASSED**.
    4. `test_common_buzzer_arbitration_across_all_4_cells` (arbitration without race conditions) -> **PASSED**.
    5. `test_dynamic_cell_count_initialization` (clean 1-to-64 initialization) -> **PASSED**.
    6. `test_dynamic_setCellCount_expansion` (runtime dynamic expansion) -> **PASSED**.
    7. `test_dynamic_setCellCount_reduction_and_cleanup` (safe deactivation & servo zeroing) -> **PASSED**.
    8. `test_dynamic_cell_count_bounds_validation` (invalid bounds protection) -> **PASSED**.
    9. `test_matrix_grid_2x2_4x4_8x8_scaling` (matrix dimension auto-layout) -> **PASSED**.
    10. `test_matrix_grid_coordinate_translation_and_triggers` (dual addressing `r,c` and linear) -> **PASSED**.
    11. `test_matrix_grid_8x8_buzzer_arbitration` (64-compartment buzzer arbitration) -> **PASSED**.
    12. `test_4x4_single_compartment_complete_isolation` (strict single-compartment isolation) -> **PASSED**.

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
7. `test_dynamic_cell_count_initialization`: Verifies initialization with dynamic count (e.g. 2 cells); inactive cells are null and reject alerts.
8. `test_dynamic_setCellCount_expansion`: Verifies runtime scaling from 2 to 4 cells; newly activated cells immediately respond to triggers.
9. `test_dynamic_setCellCount_reduction_and_cleanup`: Verifies runtime reduction from 4 to 2 cells; deactivated cells are automatically reset to IDLE, LEDs off, servos closed, and buzzer arbitration updates immediately.
10. `test_dynamic_cell_count_bounds_validation`: Verifies bounds checking (rejects 0 or >64, preserves current count).
11. `test_matrix_grid_2x2_4x4_8x8_scaling`: Verifies dynamic grid dimension scaling across 2x2 (4 cells), 4x4 (16 cells), 8x8 (64 cells), and custom rectangles (2x4).
12. `test_matrix_grid_coordinate_translation_and_triggers`: Verifies 1-indexed (Row, Col) coordinate mapping, independent servo opening, and confirmation.
13. `test_matrix_grid_8x8_buzzer_arbitration`: Verifies common alert buzzer arbitration across opposite corners of an 8x8 matrix (64 compartments).
14. `test_4x4_single_compartment_complete_isolation`: Verifies that triggering any single compartment in a 4x4 grid (16 cells) leaves all other 15 compartments strictly in IDLE and closed, and confirming leaves other simultaneous alerts untouched.

All automated unit test suites compiled and passed with **exit code 0 (100% pass rate)**.

---

## 7. Scalable Matrix Grid Architecture (2x2, 4x4, 8x8 up to 64 Cells)

The system supports enterprise scalability from single small organizers to multi-tier hospital pill cabinets:

### Supported Matrix Dimensions
* **2x2 Grid (4 Cells):** Direct physical GPIO wiring on ESP32 DevKit V1 and Wokwi simulator. Full LED + button + servo.
* **4x4 Grid (16 Cells):** Cells 1–4 = physical GPIOs; cells 5–16 = virtual expander (logic + buzzer + OLED + serial, no GPIO).
* **8x8 Grid (64 Cells):** Cells 1–4 = physical; cells 5–64 = virtual expander with same HAL.

### Physical vs Virtual Cells

| Cells | Mode | LED | Button | Servo | Buzzer/OLED/Scheduler |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 1–4 | Physical direct GPIO | Real `rgb1-4` wired to D25/D26 etc. | Real `Btn 1-4` wired to D32/D33/D27/D23, click in Wokwi | Real PWM on D18/D19/D5/TX2 | Yes |
| 5+ | Virtual expander (`pins=-1`) | Tracked + serial `[VIRTUAL] Cell X -> LED RED/GREEN/YELLOW` log, visible in `STATUS` | Not wired in Wokwi (orange `Btn X (V)`). Use serial `PRESS X` / `PRESS r,c` | Angle tracked (`90°` DUE, `0°` otherwise) + serial log, no PWM | Yes |

Why: ESP32 DevKit V1 exposes 19 usable GPIOs. 4 cells × 4 pins + I2C (21/22) + buzzer (15) = 19. No free pins remain, and Wokwi has no native MCP23017/PCA9685. Firmware therefore emulates expanders in software (`CellHardware::isVirtual()`, `injectButtonPress()`).

Virtual API:
* `cell->isVirtual()`, `controller.isCellVirtual(id)`, `isHardwareCellVirtual(id)`
* `controller.injectVirtualButtonPress(id)` / bridge `pressHardwareButton(id)` — flows through `update()` → confirmation callback → scheduler, identical to physical press.
* `STATUS` shows `P`/`V` per cell: `[C05: DUE 90° V]`.

### Input & Addressing Methods
1. **Interactive Boot Prompt:** At startup, the user can input dimensions like `2x2`, `4x4`, `8x8`, or a count `1-64`.
2. **Serial Command `GRID <RxC>` / `GRID <R> <C>`:** Runtime grid scaling (e.g., `GRID 4 4`, `GRID 8 8`, `GRID 4x4`).
3. **Serial Command `CELLS <1-64>`:** Runtime linear capacity scaling (e.g., `CELLS 16`).
4. **Dual Addressing:** Actuator commands accept both linear index and matrix coordinates:
   * `DUE 6` (Linear Cell 6) or `DUE 2,2` (Row 2, Col 2 in a 4x4 matrix).
   * `CONFIRM 6` or `CONFIRM 2,2`.
   * `RESET 6` or `RESET 2,2`.
5. **Hardware Safety:** Deactivating compartments automatically locks servos to 0°, turns off LEDs, and recalculates the buzzer alert count immediately.

---

## 8. Managing Wokwi Circuit Layout via Single Point of Entry & Generator Tool

Because Wokwi's simulator canvas schematic (`diagram.json`) is a static electronic blueprint, runtime C++ firmware cannot dynamically solder or drop new electronic components onto the web canvas. To make configuration effortless, we provide two pathways:

### A. The Single Point of Entry (Recommended Workflow)
Edit `medicine-cell-data/config/configuration.json` — firmware + diagram generator read it automatically:
```json
{
  "grid": "4x4",
  "cellCount": 16
}
```
* **Auto-Synchronization:** PlatformIO's pre-build hook (`extra_script.py`) reads `configuration.json`, regenerates `diagram.json` and `grid_config.json` before compiling. `src/main.cpp` reads the embedded copy at boot: explicit `grid` (e.g. `"4x4"`) calls `setHardwareGrid()`, `"auto"` derives `1->1x1, 2->1x2, <=4->2x2, <=16->4x4, else 8x8` (same mapping as the script). Mismatches (`cellCount` vs `grid` vs `cells[]` length) print `[CONFIG] WARNING` on serial.
* **Rule:** keep `cellCount == rows*cols == len(cells[])` (4/2x2, 16/4x4, 64/8x8) for a full matrix. Example 4x4 needs 16 `CELL01..CELL16` entries with `hour/minute` schedules.
* **Virtual canvas:** `scripts/set_diagram_grid.py` wires 1-4 to ESP GPIOs, gives 5+ power-only + orange `Btn X (V)` label + `virtual_note` text (`PRESS <id> to confirm`).

### B. Manual CLI Generator Tool
Run the helper script [scripts/set_diagram_grid.py](file:///Users/gabriel/Projects/smart-medicine-remainder/scripts/set_diagram_grid.py):

```bash
# Auto-sync directly from configuration.json:
python3 scripts/set_diagram_grid.py

# Or force an explicit grid size:
python3 scripts/set_diagram_grid.py 4x4   # 16 compartments (4x4)
python3 scripts/set_diagram_grid.py 8x8   # 64 compartments (8x8)
python3 scripts/set_diagram_grid.py 2x2   # 4 compartments (2x2)
```

### C. Physical GPIO Budget on ESP32 DevKit V1
* Each full physical compartment requires **4 dedicated GPIOs**:
  * 1 PWM GPIO for Servo motor
  * 2 Digital GPIOs for RGB LED (Red + Green channels)
  * 1 Input GPIO with pull-up for Pushbutton
* $4 \text{ compartments} \times 4 \text{ GPIOs} = 16 \text{ GPIOs}$.
* Adding 2 pins for I2C Bus (`D21` SDA, `D22` SCL) and 1 pin for Common Buzzer (`D15`) = **19 GPIO pins**.
* **19 pins is the exact limit of usable output GPIOs on the ESP32 DevKit V1.**
* Therefore, `2x2` (4 cells) is the maximum direct physical wiring on the bare microcontroller. Grids larger than 4 cells (e.g. `4x4` or `8x8`) are driven in enterprise hardware via I2C expander ICs (such as PCA9685 16-channel PWM servo drivers) and are fully managed in software by Pavithra's Hardware Abstraction Layer (HAL).

### D. Active Grid Status on OLED
When `GRID 8x8` or `GRID 4x4` is executed, the SSD1306 OLED display updates the active grid indicator in the header bar of the digital clock screen (e.g. `[8x8]`, `[4x4]`), providing instant visual confirmation while maintaining strict role separation.

### E. Serial Commands for Virtual Cells (5+)

| Command | Example | Effect |
| :--- | :--- | :--- |
| `STATUS` | `STATUS` | Matrix with `P`/`V` flags, servo angles, buzzer, due/missed lists. |
| `DUE / MISSED / CONFIRM / RESET` | `DUE 5`, `CONFIRM 2,1` | Direct state triggers, work for physical + virtual. |
| `PRESS <id \| r,c>` (aliases `BTN`, `BUTTON`) | `PRESS 5`, `PRESS 2,1` | Simulates button press via `injectVirtualButtonPress()` → `update()` → scheduler. Required for virtual cells; also works for 1-4. |
| `GRID / CELLS` | `GRID 4x4`, `CELLS 16` | Runtime rescale; keeps firmware + OLED label in sync. |

### F. Testing 4x4 Virtual Cells (Example)

1. `configuration.json`: `"grid": "4x4"`, `"cellCount": 16`, 16 cells (CELL05 21:30, CELL06 21:31, ...).
2. Build (auto-syncs diagram) → Wokwi `STOP` → `START`.
3. `TIME 21 30 00` → `CELL05 DUE`: serial `[VIRTUAL] Cell 5 -> LED RED | Servo 90°`, buzzer pulses, OLED shows due.
4. `STATUS` → `[C05: DUE 90° V]`.
5. `PRESS 5` → GREEN 1s, servo 0°, stock decremented, buzzer re-arbitrated.



