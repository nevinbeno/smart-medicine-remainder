#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <string>
#include <ArduinoJson.h>

#include "HardwareBridge.h"
#include "schedule_event_engine.hpp"
#include "time_provider.hpp"
#include "oled_notification_engine.hpp"
#include "configuration_loader.hpp"
#include "configuration_adapter.hpp"
#include "dose_manager.h"

// Embedded configuration.json text file pointers from PlatformIO
extern const uint8_t configuration_json_start[] asm("_binary_medicine_cell_data_config_configuration_json_start");
extern const uint8_t configuration_json_end[]   asm("_binary_medicine_cell_data_config_configuration_json_end");

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C Peripherals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;

// Role 2 & Role 3 Singletons
RTCTimeProvider rtcTimeProvider;
ScheduleEventEngine scheduleEngine(&rtcTimeProvider);
OledNotificationEngine oledUi;

// Role 5 Singleton: Dose & Inventory Stock Management
DoseManager doseManager(5 * 60);

// Forward declarations
void handleSerialCommands();

/**
 * @brief Scheduler Event Callback Handler
 *        Invoked whenever the Scheduling Engine emits an event
 *        (DOSE_DUE, DOSE_MISSED, DOSE_CONFIRMED, DOSE_DISMISSED, DAILY_RESET, LOW_STOCK_WARNING).
 *        Routes events to the 4-cell hardware controller (Role 4).
 *        Note: OLED UI (Role 3) receives these events independently via its own listener.
 */
void onScheduleEngineEvent(const DoseEvent& event, void* /*userData*/) {
    switch (event.type) {
        case EventType::DOSE_DUE:
            Serial.printf("\n>>> [SCHEDULER] DOSE_DUE: Cell %u (%s) - %s at %02u:%02u (Dose: %u)\n",
                          event.cellIndex + 1, event.cellId, event.medicineName,
                          event.scheduledTime.hour, event.scheduledTime.minute, event.doseQuantity);
            doseManager.addDose((uint16_t)(1000 + event.eventId), event.cellIndex + 1, rtc.now().unixtime(), event.doseQuantity);
            doseManager.update(rtc.now().unixtime());
            // Trigger 4-Cell Hardware: LED RED, Servo OPEN to 90 deg, Buzzer pulsing
            triggerHardwareMedicineDue(event.cellIndex);
            break;

        case EventType::DOSE_MISSED:
            Serial.printf("\n>>> [SCHEDULER] DOSE_MISSED: Cell %u (%s) - %s at %02u:%02u (Grace period expired)\n",
                          event.cellIndex + 1, event.cellId, event.medicineName,
                          event.scheduledTime.hour, event.scheduledTime.minute);
            doseManager.update(rtc.now().unixtime() + 3600);
            // Trigger 4-Cell Hardware: LED YELLOW (Warning), Servo CLOSED
            triggerHardwareMissedDose(event.cellIndex);
            break;

        case EventType::DOSE_CONFIRMED:
            Serial.printf("\n>>> [SCHEDULER] DOSE_CONFIRMED: Cell %u (%s) - %s (Dose %u taken)\n",
                          event.cellIndex + 1, event.cellId, event.medicineName, event.doseQuantity);
            // Stock was decremented inside scheduleEngine.confirmDose; sync with DoseManager and notify
            {
                EngineCell* cell = scheduleEngine.getCellMutable(event.cellIndex);
                uint16_t remStock = (cell) ? cell->currentStock : 0;
                uint16_t prevStock = remStock + event.doseQuantity;
                Serial.printf("[STOCK] Cell %u (%s) stock decremented: %u -> %u\n",
                              event.cellIndex + 1, event.cellId, prevStock, remStock);

                DoseResult result = doseManager.confirmCell(event.cellIndex + 1, rtc.now().unixtime());
                if (result == DoseResult::NoDueDose) {
                    doseManager.decrementStock(event.cellIndex + 1, event.doseQuantity);
                }
                if (cell) {
                    doseManager.setCellStock(event.cellIndex + 1, remStock);
                }
                const CellInventory* inv = doseManager.getCell(event.cellIndex + 1);
                if (inv) {
                    const char* resStr = "OK";
                    if (inv->stock == 0) resStr = "OUT OF STOCK (0 units left)";
                    else if (inv->isLow()) resStr = "LOW STOCK WARNING";
                    Serial.printf("[DOSE_MANAGER] Cell %u stock: %u (Threshold: %u, Status: %s)\n",
                                  event.cellIndex + 1, inv->stock, inv->lowStockThreshold, resStr);
                }
                if (remStock == 0) {
                    Serial.printf("[STOCK ALERT] Cell %u is now OUT OF STOCK (0 units left)! Refill needed.\n", event.cellIndex + 1);
                } else if (cell && remStock <= cell->lowStockThreshold) {
                    Serial.printf("[LOW STOCK] Cell %u is now LOW STOCK (%u units left)!\n", event.cellIndex + 1, remStock);
                }
            }
            // Trigger 4-Cell Hardware: LED GREEN for 1s, Servo CLOSED
            triggerHardwareDoseConfirmed(event.cellIndex);
            break;

        case EventType::DOSE_DISMISSED:
            Serial.printf("\n>>> [SCHEDULER] DOSE_DISMISSED: Cell %u (%s) - %s\n",
                          event.cellIndex + 1, event.cellId, event.medicineName);
            doseManager.acknowledgeMissedAlerts();
            // Clear warning state for this cell -> LED OFF
            triggerHardwareReset(event.cellIndex);
            break;

        case EventType::DAILY_RESET:
            Serial.println("\n>>> [SCHEDULER] DAILY_RESET: Midnight rollover! Refreshing daily schedules.");
            triggerHardwareResetAll();
            break;

        case EventType::LOW_STOCK_WARNING:
            if (event.doseQuantity == 0) {
                Serial.printf("\n>>> [STOCK ALERT] Cell %u (%s) - %s is OUT OF STOCK (0 units)! Scheduled dose skipped. Please RESTOCK.\n",
                              event.cellIndex + 1, event.cellId, event.medicineName);
                triggerHardwareMissedDose(event.cellIndex); // Warning LED (yellow), servo stays CLOSED
            } else {
                Serial.printf("\n>>> [SCHEDULER] LOW_STOCK_WARNING: Cell %u (%s) - %s (Remaining Stock: %u)\n",
                              event.cellIndex + 1, event.cellId, event.medicineName, event.doseQuantity);
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Outbound Event Handler: Physical Button Press on a Cell
 *        Invoked by the debounced button logic when a patient presses the button on a cell.
 *        Passes the confirmation to the scheduler, which confirms active DUE doses
 *        or dismisses active MISSED dose notifications, updating the OLED and hardware.
 */
void onHardwareDoseConfirmed(uint8_t cellId) {
    uint8_t cellNum = cellId + 1;
    Serial.printf("[CONFIRM ACTION] Button pressed for Cell %u. Checking stock and scheduler...\n", cellNum);

    EngineCell* cell = scheduleEngine.getCellMutable(cellId);
    const CellInventory* inv = doseManager.getCell(cellNum);
    uint16_t currentStock = (cell) ? cell->currentStock : (inv ? inv->stock : 0);

    if (currentStock == 0) {
        Serial.printf("[STOCK ALERT] Cell %u button pressed, but stock is 0 (OUT OF STOCK)! Cannot dispense.\n", cellNum);
        triggerHardwareReset(cellId);
        return;
    }

    bool handled = scheduleEngine.handleCellButtonPress(cellId);
    if (!handled) {
        // If not handled by scheduler, check if doseManager has an active due dose
        DoseResult res = doseManager.confirmCell(cellNum, rtc.now().unixtime());
        if (res == DoseResult::Confirmed || res == DoseResult::ConfirmedLowStock || res == DoseResult::ConfirmedStockEmpty) {
            uint16_t rem = doseManager.getCell(cellNum)->stock;
            if (cell) {
                cell->currentStock = rem;
            }
            Serial.printf("[DOSE_MANAGER] Cell %u confirmed via DoseManager! Remaining stock: %u\n",
                          cellNum, rem);
            triggerHardwareDoseConfirmed(cellId);
        } else {
            Serial.printf("[CONFIRM] Cell %u has no active due dose to confirm (Stock unchanged: %u units).\n",
                          cellNum, currentStock);
            triggerHardwareReset(cellId);
        }
    }
}

void setup() {
    Serial.begin(115200);

    // 1. Initialize I2C Bus (D21 SDA, D22 SCL preserved)
    Wire.begin(21, 22);

    // 2. Initialize Hardware RTC
    if (!rtc.begin()) {
        Serial.println("RTC initialization failed!");
        while (true);
    }
    if (!rtc.isrunning()) {
        Serial.println("RTC was not running, setting to compile time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("RTC initialized!");

    // 3. Load dynamic configuration from embedded configuration.json (No hardcoding!)
    size_t configLen = configuration_json_end - configuration_json_start;
    std::string configJson(reinterpret_cast<const char*>(configuration_json_start), configLen);

    RuntimeConfiguration runtimeConfig;
    uint8_t targetCellCount = 4;
    if (loadConfiguration(configJson.c_str(), runtimeConfig)) {
        Serial.printf("[CONFIG] Successfully loaded configuration.json (%u cells configured):\n", runtimeConfig.cellCount);
        for (uint8_t i = 0; i < runtimeConfig.cellCount; i++) {
            RuntimeCell& c = runtimeConfig.cells[i];
            Serial.printf("  Cell %u [%s]: %s (Stock: %u)\n",
                          i + 1, c.cellId, c.hasMedicine ? c.medicine.name : "EMPTY", c.currentStock);
            doseManager.addCell(i + 1, c.hasMedicine ? c.medicine.name : "Cell", c.currentStock, c.medicine.lowStockThreshold);
            DateTime now = rtc.now();
            for (uint8_t s = 0; s < c.medicine.scheduleCount; s++) {
                uint8_t sh = c.medicine.schedules[s].hour;
                uint8_t sm = c.medicine.schedules[s].minute;
                uint16_t sq = c.medicine.schedules[s].doseQuantity;
                Serial.printf("    - Schedule: %02u:%02u (Dose: %u)\n", sh, sm, sq);
                DateTime schedDt(now.year(), now.month(), now.day(), sh, sm, 0);
                uint16_t doseId = (uint16_t)((i + 1) * 100 + s + 1);
                doseManager.addDose(doseId, i + 1, schedDt.unixtime(), sq);
            }
        }
        doseManager.update(rtc.now().unixtime());
        if (runtimeConfig.cellCount > 0 && runtimeConfig.cellCount <= MAX_CELLS) {
            targetCellCount = runtimeConfig.cellCount;
        }
    } else {
        Serial.println("[CONFIG] ERROR: Failed to parse configuration.json! Defaulting to 4 cells.");
    }

    Serial.printf("[INIT] Starting system with %u active cell(s).\n\n", targetCellCount);

    // 4. Initialize Role 4: Cell Hardware & Alert Control with dynamic cell count
    initHardwareBridge(onHardwareDoseConfirmed, targetCellCount);

    // Check if configuration.json specifies an explicit hardware grid (e.g. "4x4", "8x8")
    // If "auto", derive grid from cellCount with the same mapping as scripts/set_diagram_grid.py
    // so firmware + Wokwi canvas stay in sync automatically.
    JsonDocument cfgDoc;
    if (!deserializeJson(cfgDoc, configJson.c_str())) {
        const char* explicitGrid = cfgDoc["grid"] | "auto";
        if (explicitGrid && strcmp(explicitGrid, "auto") != 0) {
            int gr = 0, gc = 0;
            if (sscanf(explicitGrid, "%dx%d", &gr, &gc) == 2 || sscanf(explicitGrid, "%dX%d", &gr, &gc) == 2) {
                if (gr >= 1 && gr <= 8 && gc >= 1 && gc <= 8 && (gr * gc) <= 64) {
                    setHardwareGrid((uint8_t)gr, (uint8_t)gc);
                    Serial.printf("[CONFIG] Explicit hardware matrix configured to [%ux%u] = %u compartments!\n",
                                  gr, gc, gr * gc);
                    if ((uint8_t)(gr * gc) != targetCellCount) {
                        Serial.printf("[CONFIG] NOTE: grid %ux%u (%u) overrides cellCount %u. "
                                      "Set both to match (e.g. 4/2x2, 16/4x4) for full matrix.\n",
                                      gr, gc, gr * gc, targetCellCount);
                    }
                }
            }
        } else {
            // auto: 1->1x1, 2->1x2, <=4->2x2, <=16->4x4, else 8x8 (matches diagram generator)
            uint8_t ar = 1, ac = targetCellCount;
            if (targetCellCount <= 1) { ar = 1; ac = 1; }
            else if (targetCellCount <= 2) { ar = 1; ac = 2; }
            else if (targetCellCount <= 4) { ar = 2; ac = 2; }
            else if (targetCellCount <= 16) { ar = 4; ac = 4; }
            else { ar = 8; ac = 8; }
            setHardwareGrid(ar, ac);
            Serial.printf("[CONFIG] Auto grid derived from cellCount %u -> [%ux%u].\n",
                          targetCellCount, ar, ac);
        }
        size_t cellsInJson = cfgDoc["cells"].is<JsonArray>() ? cfgDoc["cells"].as<JsonArray>().size() : 0;
        if (cellsInJson != getHardwareCellCount()) {
            Serial.printf("[CONFIG] WARNING: cells[] has %u entries but hardware has %u active. "
                          "Align cellCount/grid/cells[] in configuration.json.\n",
                          (unsigned)cellsInJson, getHardwareCellCount());
        }
    }

    Serial.printf("Role 4: Cell Hardware & Alert Control initialized with %u active cells (Grid [%ux%u])!\n",
                  getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
    if (getHardwareCellCount() > 4) {
        Serial.println("[CONFIG] Cells 1-4 = physical GPIO; Cells 5+ = virtual expander (LED/servo logged, PRESS <id> to confirm).");
    }

    // 5. Initialize Role 2: Scheduling & Time/Event Engine
    scheduleEngine.setTimeProvider(&rtcTimeProvider);
    scheduleEngine.setGracePeriodMinutes(5); // 5-minute grace window for alerts before marking MISSED
    scheduleEngine.registerEventCallback(onScheduleEngineEvent);
    if (runtimeConfig.cellCount > 0) {
        ConfigurationAdapter::loadIntoEngine(runtimeConfig, scheduleEngine);
        Serial.println("[CONFIG] Schedules dynamically loaded into Scheduling Engine!");
    }

    // 6. Initialize Role 3: OLED UI & Notification Engine
    if (!oledUi.begin(display, scheduleEngine)) {
        Serial.println("OLED UI initialization failed!");
        while (true);
    }
    char initGridLabel[16];
    snprintf(initGridLabel, sizeof(initGridLabel), "[%ux%u]", getHardwareGridRows(), getHardwareGridCols());
    oledUi.setGridLabel(initGridLabel);
    Serial.println("Role 3: OLED UI & Notification Engine initialized!");

    Serial.println("=================================================================");
    Serial.println(" Smart Medicine Reminder System - Scalable Matrix Hardware Active");
    Serial.printf(" Matrix Grid: [%ux%u] | Cells: %u | Common Buzzer: ON\n",
                  getHardwareGridRows(), getHardwareGridCols(), getHardwareCellCount());
    Serial.println("=================================================================");
    Serial.println("Serial Commands:");
    Serial.println("  STATUS              - Print full visual matrix hardware & event status");
    Serial.println("  GRID <RxC>          - Scale matrix grid (e.g. GRID 2x2, GRID 4x4, GRID 8x8)");
    Serial.println("  CELLS <1-64>        - Dynamically set number of active cells");
    Serial.println("  CONFIRM <id | r,c>  - Confirm dose for Cell (e.g. CONFIRM 1 or CONFIRM 2,1)");
    Serial.println("  PRESS <id | r,c>    - Simulate button press (required for virtual Cells 5+)");
    Serial.println("  DUE <id | r,c>      - Manually trigger MEDICINE_DUE for Cell (or row,col)");
    Serial.println("  MISSED <id | r,c>   - Manually trigger MISSED_DOSE for Cell (or row,col)");
    Serial.println("  RESET <id | r,c>    - Reset Cell (or row,col) to IDLE");
    Serial.println("  RESET               - Reset all active cells & dismiss all reminders");
    Serial.println("  LOWSTOCK <1-4>      - Simulate low stock for Cell X (OLED alert)");
    Serial.println("  RESTOCK <1-N> <qty> - Refill medicine stock using DoseManager");
    Serial.println("  TIME <HH> <MM> [SS] - Set RTC time (e.g., TIME 07 59 55)");
    Serial.println("  GRACE <minutes>     - Set missed dose grace period in minutes");
    Serial.println("Or click buttons 'Btn 1' - 'Btn 4' on virtual board! For Cells 5+, use PRESS <id>.");
    Serial.println("=================================================================");
}

// Periodic heartbeat timer
static uint32_t lastSerialHeartbeat = 0;

void loop() {
    // -------------------------------------------------------------
    // 1. Role 4: Cell Hardware & Alert Control (Run continuously)
    //    Debounces buttons, manages RGB LEDs, servos, buzzer cadence.
    // -------------------------------------------------------------
    updateHardwareBridge();

    // -------------------------------------------------------------
    // 2. Role 2: Scheduling & Time/Event Engine
    //    Polls RTC time, detects due schedules, tracks grace periods,
    //    handles simultaneous due events and midnight rollovers.
    // -------------------------------------------------------------
    scheduleEngine.update();

    // -------------------------------------------------------------
    // 3. Role 3: OLED UI & Notification Engine
    //    Non-blocking: renders clock screen, active due notifications,
    //    missed dose warnings, confirmation confirmations, rotating multi-doses.
    // -------------------------------------------------------------
    oledUi.update();

    // -------------------------------------------------------------
    // 4. Role 5: Dose, Stock & Confirmation Manager
    // -------------------------------------------------------------
    doseManager.update(rtc.now().unixtime());

    // -------------------------------------------------------------
    // 5. Interactive Serial Command Processing
    // -------------------------------------------------------------
    handleSerialCommands();

    // -------------------------------------------------------------
    // 5. Periodic Heartbeat Log to Serial Monitor (Every 5 seconds)
    // -------------------------------------------------------------
    uint32_t currentMillis = millis();
    if (currentMillis - lastSerialHeartbeat >= 5000) {
        lastSerialHeartbeat = currentMillis;

        DateTime now = rtc.now();
        Serial.printf("[HEARTBEAT] RTC: %02u:%02u:%02u | Due: %u | Missed: %u | Alerting Cells: %u | Buzzer: %s\n",
                      now.hour(), now.minute(), now.second(),
                      scheduleEngine.getDueEventCount(),
                      scheduleEngine.getMissedEventCount(),
                      getHardwareActiveAlertCount(),
                      getHardwareActiveAlertCount() > 0 ? "PULSING" : "SILENT");
    }
}

/**
 * @brief Parse interactive test commands from Serial input.
 */
void handleSerialCommands() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) return;

    String upperLine = line;
    upperLine.toUpperCase();

    if (upperLine == "STATUS") {
        printHardwareStatus();

        DateTime now = rtc.now();
        Serial.printf("Current RTC Time    : %02u:%02u:%02u\n", now.hour(), now.minute(), now.second());
        Serial.printf("Grace Period        : %u minutes\n", scheduleEngine.getGracePeriodMinutes());
        Serial.printf("Active Due Events   : %u\n", scheduleEngine.getDueEventCount());
        for (uint8_t i = 0; i < scheduleEngine.getDueEventCount(); i++) {
            const DoseEvent* ev = scheduleEngine.getDueEvent(i);
            if (ev) {
                Serial.printf("  - DUE: Cell %u (%s), %s, Sched: %02u:%02u, Qty: %u\n",
                              ev->cellIndex + 1, ev->cellId, ev->medicineName,
                              ev->scheduledTime.hour, ev->scheduledTime.minute, ev->doseQuantity);
            }
        }
        Serial.printf("Active Missed Events: %u\n", scheduleEngine.getMissedEventCount());
        for (uint8_t i = 0; i < scheduleEngine.getMissedEventCount(); i++) {
            const DoseEvent* ev = scheduleEngine.getMissedEvent(i);
            if (ev) {
                Serial.printf("  - MISSED: Cell %u (%s), %s, Sched: %02u:%02u\n",
                              ev->cellIndex + 1, ev->cellId, ev->medicineName,
                              ev->scheduledTime.hour, ev->scheduledTime.minute);
            }
        }
        Serial.println("Inventory Stock Status (DoseManager + Scheduler):");
        for (uint8_t i = 1; i <= getHardwareCellCount(); i++) {
            const CellInventory* inv = doseManager.getCell(i);
            const EngineCell* cell = scheduleEngine.getCell(i - 1);
            uint16_t stock = (cell) ? cell->currentStock : (inv ? inv->stock : 0);
            uint16_t threshold = (inv) ? inv->lowStockThreshold : (cell ? cell->lowStockThreshold : 0);
            const char* statusStr = "OK";
            if (stock == 0) statusStr = "OUT OF STOCK [0 LEFT]";
            else if (stock <= threshold) statusStr = "LOW STOCK WARNING";
            Serial.printf("  - Cell %u (%s): Stock %u | Threshold %u | Status: %s\n",
                          i, inv ? inv->medicineName.c_str() : (cell ? cell->medicineName : "EMPTY"),
                          stock, threshold, statusStr);
        }
        if (doseManager.hasMissedAlert()) {
            Serial.printf("  [ALERT] DoseManager has pending missed dose alerts (Count: %u)\n",
                          (unsigned)doseManager.missedDoseCount());
        }
        Serial.println("----------------------------------------\n");
        return;
    }

    if (upperLine.startsWith("TIME ")) {
        // Syntax: TIME <HH> <MM> [SS]
        String args = line.substring(5);
        args.trim();
        int h = 0, m = 0, s = 0;
        int firstSpace = args.indexOf(' ');
        if (firstSpace > 0) {
            h = args.substring(0, firstSpace).toInt();
            String rest = args.substring(firstSpace + 1);
            rest.trim();
            int secondSpace = rest.indexOf(' ');
            if (secondSpace > 0) {
                m = rest.substring(0, secondSpace).toInt();
                s = rest.substring(secondSpace + 1).toInt();
            } else {
                m = rest.toInt();
                s = 0;
            }
            if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
                rtc.adjust(DateTime(2026, 9, 28, h, m, s));
                oledUi.requestRedraw();
                Serial.printf("[COMMAND] Set RTC Time to %02d:%02d:%02d\n", h, m, s);
                return;
            }
        }
        Serial.println("Invalid format. Use: TIME <HH> <MM> [SS] (e.g., TIME 07 59 55)");
        return;
    }

    auto parseCellArg = [](const String& arg) -> int {
        int commaIdx = arg.indexOf(',');
        if (commaIdx > 0) {
            int r = arg.substring(0, commaIdx).toInt();
            int c = arg.substring(commaIdx + 1).toInt();
            int8_t idx = getHardwareCellIndex((uint8_t)r, (uint8_t)c);
            if (idx >= 0) return idx + 1; // return 1-indexed cell number
            return -1;
        }
        return arg.toInt();
    };

    if (upperLine.startsWith("GRID ")) {
        String args = line.substring(5);
        args.trim();
        int r = 0, c = 0;
        int xPos = args.indexOf('x');
        if (xPos < 0) xPos = args.indexOf('X');
        int spacePos = args.indexOf(' ');

        if (xPos > 0) {
            r = args.substring(0, xPos).toInt();
            c = args.substring(xPos + 1).toInt();
        } else if (spacePos > 0) {
            r = args.substring(0, spacePos).toInt();
            c = args.substring(spacePos + 1).toInt();
        }

        if (r >= 1 && r <= 8 && c >= 1 && c <= 8 && (r * c) <= 64) {
            bool ok = setHardwareGrid((uint8_t)r, (uint8_t)c);
            if (ok) {
                Serial.printf("[COMMAND] Hardware grid scaled to [%ux%u] = %u active cells!\n", r, c, r * c);
                printHardwareStatus();

                char gridStr[16];
                snprintf(gridStr, sizeof(gridStr), "[%ux%u]", r, c);
                oledUi.setGridLabel(gridStr);
            } else {
                Serial.printf("[COMMAND] Failed to set hardware grid [%ux%u].\n", r, c);
            }
        } else {
            Serial.println("Invalid grid dimensions. Max: 8x8 (64 cells). E.g.: GRID 2 2, GRID 4 4, GRID 8 8, GRID 4x4");
        }
        return;
    }

    if (upperLine.startsWith("CELLS ") || upperLine.startsWith("SET_CELLS ")) {
        int spaceIdx = upperLine.indexOf(' ');
        int count = line.substring(spaceIdx + 1).toInt();
        if (count >= 1 && count <= 64) {
            bool ok = setHardwareCellCount((uint8_t)count);
            if (ok) {
                Serial.printf("[COMMAND] Dynamic active cell count updated to %d cell(s) (Grid [%ux%u])!\n",
                              count, getHardwareGridRows(), getHardwareGridCols());
                printHardwareStatus();

                char gridStr[16];
                snprintf(gridStr, sizeof(gridStr), "[%ux%u]", getHardwareGridRows(), getHardwareGridCols());
                oledUi.setGridLabel(gridStr);
            } else {
                Serial.printf("[COMMAND] Failed to set cell count to %d.\n", count);
            }
        } else {
            Serial.println("Invalid cell count. Range: 1 to 64 cells (e.g., CELLS 16).");
        }
        return;
    }

    if (upperLine.startsWith("GRACE ")) {
        int minutes = line.substring(6).toInt();
        if (minutes > 0 && minutes <= 120) {
            scheduleEngine.setGracePeriodMinutes(minutes);
            Serial.printf("[COMMAND] Grace period set to %d minutes\n", minutes);
        } else {
            Serial.println("Invalid grace period. Range: 1 to 120 minutes.");
        }
        return;
    }

    if (upperLine.startsWith("PRESS ") || upperLine.startsWith("BTN ") || upperLine.startsWith("BUTTON ")) {
        int spaceIdx = upperLine.indexOf(' ');
        String arg = line.substring(spaceIdx + 1);
        arg.trim();
        int cellNum = parseCellArg(arg);
        if (cellNum >= 1 && cellNum <= (int)getHardwareCellCount()) {
            uint8_t r = 0, c = 0;
            getHardwareCellGridCoords(cellNum - 1, r, c);
            Serial.printf("[COMMAND] Simulating button PRESS for Cell %d (Row %u, Col %u)%s\n",
                          cellNum, r, c,
                          isHardwareCellVirtual(cellNum - 1) ? " [VIRTUAL expander]" : " [PHYSICAL]");
            if (!pressHardwareButton(cellNum - 1)) {
                Serial.printf("[COMMAND] Failed to inject press for Cell %d.\n", cellNum);
            } else {
                Serial.println("[HINT] Press flows through update() -> scheduler; check STATUS / OLED / buzzer.");
            }
        } else {
            Serial.printf("Invalid cell: '%s' (Available: 1 to %u, or Row,Col up to %u,%u)\n",
                          arg.c_str(), getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
        }
        return;
    }

    if (upperLine.startsWith("DUE ")) {
        String arg = line.substring(4);
        arg.trim();
        int cellNum = parseCellArg(arg);
        if (cellNum >= 1 && cellNum <= (int)getHardwareCellCount()) {
            uint8_t r = 0, c = 0;
            getHardwareCellGridCoords(cellNum - 1, r, c);

            EngineCell* cell = scheduleEngine.getCellMutable(cellNum - 1);
            const CellInventory* inv = doseManager.getCell(cellNum);
            uint16_t currentStock = (cell) ? cell->currentStock : (inv ? inv->stock : 0);

            if (currentStock == 0) {
                Serial.printf("[STOCK ALERT] Cannot trigger DUE for Cell %d (Row %u, Col %u): OUT OF STOCK (Stock: 0)!\n",
                              cellNum, r, c);
                Serial.printf("[STOCK ALERT] Servo kept CLOSED. Please use 'RESTOCK %d <qty>' first.\n", cellNum);
                triggerHardwareMissedDose(cellNum - 1); // Yellow warning LED, servo stays CLOSED
                return;
            }

            Serial.printf("[COMMAND] Triggering MEDICINE_DUE for Cell %d (Row %u, Col %u) - Stock: %u\n",
                          cellNum, r, c, currentStock);
            bool ok = scheduleEngine.triggerDueEvent(cellNum - 1);
            if (!ok) {
                uint16_t reqQty = 1;
                if (cell && cell->scheduleCount > 0) {
                    reqQty = cell->schedules[0].doseQuantity;
                }
                doseManager.addDose((uint16_t)(1000 + cellNum), cellNum, rtc.now().unixtime(), reqQty);
                doseManager.update(rtc.now().unixtime());
            }
        } else {
            Serial.printf("Invalid cell: '%s' (Available: 1 to %u, or Row,Col up to %u,%u)\n",
                          arg.c_str(), getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
        }
        return;
    } else if (upperLine.startsWith("MISSED ")) {
        String arg = line.substring(7);
        arg.trim();
        int cellNum = parseCellArg(arg);
        if (cellNum >= 1 && cellNum <= (int)getHardwareCellCount()) {
            uint8_t r = 0, c = 0;
            getHardwareCellGridCoords(cellNum - 1, r, c);
            Serial.printf("[COMMAND] Triggering MISSED_DOSE for Cell %d (Row %u, Col %u)\r\n", cellNum, r, c);
            triggerHardwareMissedDose(cellNum - 1);
            scheduleEngine.triggerMissedEvent(cellNum - 1);
            oledUi.requestRedraw();
        } else {
            Serial.printf("Invalid cell: '%s' (Available: 1 to %u, or Row,Col up to %u,%u)\n",
                          arg.c_str(), getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
        }
    } else if (upperLine.startsWith("CONFIRM ")) {
        String arg = line.substring(8);
        arg.trim();
        int cellNum = parseCellArg(arg);
        if (cellNum >= 1 && cellNum <= (int)getHardwareCellCount()) {
            uint8_t r = 0, c = 0;
            getHardwareCellGridCoords(cellNum - 1, r, c);

            EngineCell* cell = scheduleEngine.getCellMutable(cellNum - 1);
            const CellInventory* inv = doseManager.getCell(cellNum);
            uint16_t currentStock = (cell) ? cell->currentStock : (inv ? inv->stock : 0);

            if (currentStock == 0) {
                Serial.printf("[STOCK ERROR] Cannot confirm Cell %d (Row %u, Col %u): Stock is 0 (OUT OF STOCK)!\n",
                              cellNum, r, c);
                Serial.printf("[STOCK ERROR] Dose cannot be taken from an empty compartment. Refill with 'RESTOCK %d <qty>'.\n",
                              cellNum);
                return;
            }

            Serial.printf("[COMMAND] Confirming dose for Cell %d (Row %u, Col %u)\n", cellNum, r, c);
            bool handled = scheduleEngine.handleCellButtonPress(cellNum - 1);
            if (!handled) {
                // Check if doseManager has an active due occurrence
                DoseResult res = doseManager.confirmCell((uint8_t)cellNum, rtc.now().unixtime());
                if (res == DoseResult::Confirmed || res == DoseResult::ConfirmedLowStock || res == DoseResult::ConfirmedStockEmpty) {
                    uint16_t rem = doseManager.getCell((uint8_t)cellNum)->stock;
                    if (cell) {
                        cell->currentStock = rem;
                    }
                    Serial.printf("[DOSE_MANAGER] Cell %d confirmed via DoseManager! Remaining stock: %u\n", cellNum, rem);
                    triggerHardwareDoseConfirmed(cellNum - 1);
                } else {
                    Serial.printf("[COMMAND] Cell %d has no active due dose to confirm (Stock unchanged: %u units).\n",
                                  cellNum, currentStock);
                }
            }
        } else {
            Serial.printf("Invalid cell: '%s' (Available: 1 to %u, or Row,Col up to %u,%u)\n",
                          arg.c_str(), getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
        }
        return;
    } else if (upperLine.startsWith("RESET ")) {
        String arg = line.substring(6);
        arg.trim();
        int cellNum = parseCellArg(arg);
        if (cellNum >= 1 && cellNum <= (int)getHardwareCellCount()) {
            uint8_t r = 0, c = 0;
            getHardwareCellGridCoords(cellNum - 1, r, c);
            Serial.printf("[COMMAND] Resetting Cell %d (Row %u, Col %u) to IDLE\n", cellNum, r, c);
            triggerHardwareReset(cellNum - 1);
            scheduleEngine.dismissMissedDose(cellNum - 1);
        } else {
            Serial.printf("Invalid cell: '%s' (Available: 1 to %u, or Row,Col up to %u,%u)\n",
                          arg.c_str(), getHardwareCellCount(), getHardwareGridRows(), getHardwareGridCols());
        }
    } else if (upperLine == "RESET") {
        Serial.printf("[COMMAND] Resetting all %u cells to IDLE & dismissing missed reminders\n", getHardwareCellCount());
        triggerHardwareResetAll();
        scheduleEngine.dismissAllMissedDoses();
    } else if (upperLine.startsWith("LOWSTOCK ")) {
        // Manually set a cell's stock to 1 to trigger a LOW_STOCK_WARNING on next engine tick.
        int cellNum = line.substring(9).toInt();
        if (cellNum >= 1 && cellNum <= (int)scheduleEngine.getCellCount()) {
            EngineCell* cell = scheduleEngine.getCellMutable(cellNum - 1);
            if (cell) {
                cell->currentStock = 1;  // Force stock below any threshold
                doseManager.setCellStock((uint8_t)cellNum, 1);
                Serial.printf("[COMMAND] Cell %d stock forced to 1 (LOW_STOCK_WARNING expected on next update)\n", cellNum);
            }
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, scheduleEngine.getCellCount());
        }
    } else if (upperLine.startsWith("EMPTY ") || upperLine.startsWith("ZEROSTOCK ")) {
        int spaceIdx = upperLine.indexOf(' ');
        int cellNum = line.substring(spaceIdx + 1).toInt();
        if (cellNum >= 1 && cellNum <= (int)scheduleEngine.getCellCount()) {
            EngineCell* cell = scheduleEngine.getCellMutable(cellNum - 1);
            if (cell) {
                cell->currentStock = 0;
                doseManager.setCellStock((uint8_t)cellNum, 0);
                Serial.printf("[COMMAND] Cell %d stock forced to 0 (OUT OF STOCK)\n", cellNum);
                triggerHardwareMissedDose(cellNum - 1); // Yellow warning LED, servo CLOSED
            }
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, scheduleEngine.getCellCount());
        }
    } else if (upperLine.startsWith("RESTOCK ")) {
        int cellNum = 0, units = 0;
        if (sscanf(line.c_str() + 8, "%d %d", &cellNum, &units) == 2 && cellNum >= 1 && units > 0) {
            bool ok = doseManager.restockCell((uint8_t)cellNum, (uint16_t)units);
            if (cellNum <= (int)scheduleEngine.getCellCount()) {
                EngineCell* cell = scheduleEngine.getCellMutable(cellNum - 1);
                if (cell) {
                    cell->currentStock += units;
                    doseManager.setCellStock((uint8_t)cellNum, cell->currentStock);
                }
            }
            if (ok) {
                const CellInventory* inv = doseManager.getCell((uint8_t)cellNum);
                uint16_t s = inv ? inv->stock : 0;
                Serial.printf("[COMMAND] Cell %d restocked +%d units. New stock: %u (Status: %s)\n",
                              cellNum, units, s,
                              (s == 0) ? "OUT OF STOCK" : ((inv && inv->isLow()) ? "LOW STOCK" : "OK"));
                triggerHardwareReset(cellNum - 1);
            } else {
                Serial.printf("[COMMAND] Failed to restock Cell %d (invalid cell or overflow)\n", cellNum);
            }
        } else {
            Serial.println("Usage: RESTOCK <cellNum> <units> (e.g. RESTOCK 1 10)");
        }
    } else {
        Serial.printf("Unknown command: '%s'. Try: STATUS, GRID, CELLS, DUE, CONFIRM, PRESS, MISSED, RESET, LOWSTOCK, EMPTY, RESTOCK, TIME, GRACE\n", line.c_str());
    }
}