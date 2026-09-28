#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include "CellHardwareEvents.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

RTC_DS1307 rtc;

// Global instance of the 4-cell hardware controller (Role 4)
CellHardwareController cellHardware(ACTIVE_CELL_COUNT);

// Forward declarations
void handleSerialCommands();

/**
 * @brief Outbound Event Handler: DOSE_CONFIRMED
 *        Invoked when physical confirm button on a cell is debounced and pressed.
 *        Role 5 (Dose/Stock Management) attaches here to decrement medicine stock.
 *        NOTE: In accordance with Role 4 boundary, this function does NOT decrement stock!
 */
void onDoseConfirmed(uint8_t cellId) {
    Serial.printf("\n>>> [EVENT OUTBOUND] DOSE_CONFIRMED for Cell %u! (Dispatched to Role 5)\n", cellId + 1);
    Serial.printf("    Hardware Action: Cell %u RGB is GREEN for 1s. Servo %u is CLOSED.\n\n", cellId + 1, cellId + 1);
}

/**
 * @brief State Change Observer Callback
 *        Invoked whenever any cell undergoes a state machine transition.
 *        Role 3 (OLED UI & Notification Engine) can observe this to update screen.
 */
void onCellStateChanged(uint8_t cellId, CellState oldState, CellState newState) {
    Serial.printf("[STATE CHANGE] Cell %u: %s -> %s\n",
                  cellId + 1, cellStateToString(oldState), cellStateToString(newState));
}

void setup() {
    Serial.begin(115200);

    // Start I2C (Pins D21, D22 preserved for OLED and RTC)
    Wire.begin(21, 22);

    // Start OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED initialization failed!");
        while (true);
    }

    // Start RTC
    if (!rtc.begin()) {
        Serial.println("RTC initialization failed!");
        while (true);
    }

    Serial.println("OLED initialized!");
    Serial.println("RTC initialized!");

    // Initialize Role 4: Cell Hardware & Alert Control (4 Cells, 4 Servos, Common Buzzer)
    cellHardware.setConfirmationCallback(onDoseConfirmed);
    cellHardware.setStateChangedCallback(onCellStateChanged);
    cellHardware.begin();

    Serial.println("=================================================");
    Serial.println(" Smart Medicine Reminder - 4-Cell Hardware System");
    Serial.println(" Role 4: Cell Hardware & Alert Control Active");
    Serial.println("=================================================");
    Serial.println("Configuration:");
    Serial.println("  4 Independent Cells (Each has RGB LED, Button, Servo)");
    Serial.println("  1 Common Piezo Buzzer (Arbitrated across all 4 cells)");
    Serial.println("-------------------------------------------------");
    Serial.println("Interactive Commands via Serial Monitor:");
    Serial.println("  DUE <1-4>     - Trigger MEDICINE_DUE for Cell X (RED + Servo OPEN + Buzzer)");
    Serial.println("  CONFIRM <1-4> - Simulate confirmation button for Cell X");
    Serial.println("  MISSED <1-4>  - Trigger MISSED_DOSE for Cell X (YELLOW + Servo CLOSED)");
    Serial.println("  RESET <1-4>   - Reset Cell X to IDLE");
    Serial.println("  RESET         - Reset all 4 cells to IDLE");
    Serial.println("  STATUS        - Print hardware status of all 4 cells");
    Serial.println("Or click buttons 'Btn 1' - 'Btn 4' on virtual board!");
    Serial.println("=================================================");
}

// Non-blocking timer for OLED/RTC display update
static uint32_t lastDisplayUpdate = 0;

void loop() {
    // -------------------------------------------------------------
    // 1. Role 4: Cell Hardware & Alert Control (Run continuously)
    // -------------------------------------------------------------
    cellHardware.update();

    // -------------------------------------------------------------
    // 2. Interactive Serial Command Processing (Testing & Simulation)
    // -------------------------------------------------------------
    handleSerialCommands();

    // -------------------------------------------------------------
    // 3. OLED & RTC Update (Non-blocking: replaces delay(1000))
    // -------------------------------------------------------------
    uint32_t currentMillis = millis();
    if (currentMillis - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = currentMillis;

        DateTime now = rtc.now();

        // Serial Monitor Time Log
        Serial.print("Time: ");
        if (now.hour() < 10) Serial.print("0");
        Serial.print(now.hour());
        Serial.print(":");
        if (now.minute() < 10) Serial.print("0");
        Serial.print(now.minute());
        Serial.print(":");
        if (now.second() < 10) Serial.print("0");
        Serial.println(now.second());

        // OLED Update
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("SMART MEDICINE (4C)");

        display.setCursor(0, 10);
        display.println("CURRENT TIME");

        display.setTextSize(2);
        display.setCursor(10, 20);
        if (now.hour() < 10) display.print("0");
        display.print(now.hour());
        display.print(":");
        if (now.minute() < 10) display.print("0");
        display.print(now.minute());
        display.print(":");
        if (now.second() < 10) display.print("0");
        display.print(now.second());

        // 4-Cell Status line on OLED
        display.setTextSize(1);
        display.setCursor(0, 42);
        auto stateCode = [](CellState s) -> const char* {
            switch (s) {
                case CELL_STATE_ALERTING:  return "RED";
                case CELL_STATE_CONFIRMED: return "GRN";
                case CELL_STATE_WARNING:   return "YEL";
                case CELL_STATE_IDLE:
                default:                   return "OFF";
            }
        };

        display.printf("1:%s 2:%s",
            stateCode(cellHardware.getCellState(0)),
            stateCode(cellHardware.getCellState(1))
        );

        display.setCursor(0, 52);
        display.printf("3:%s 4:%s BZ:%s",
            stateCode(cellHardware.getCellState(2)),
            stateCode(cellHardware.getCellState(3)),
            cellHardware.getActiveAlertCount() > 0 ? "ON" : "OFF"
        );

        display.display();
    }
}

/**
 * @brief Parse interactive test commands from Serial input.
 */
void handleSerialCommands() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    line.toUpperCase();

    if (line.length() == 0) return;

    if (line == "STATUS") {
        Serial.println("\n--- 4-CELL HARDWARE STATUS ---");
        for (uint8_t i = 0; i < cellHardware.getCellCount(); i++) {
            CellHardware* c = cellHardware.getCell(i);
            Serial.printf("Cell %u: %-22s | Servo %u: %s (%u deg)\n",
                          i + 1,
                          cellStateToString(cellHardware.getCellState(i)),
                          i + 1,
                          (c && c->getServo().isOpen()) ? "OPEN" : "CLOSED",
                          c ? c->getServo().getCurrentAngle() : 0);
        }
        Serial.printf("Active Alert Count : %u\n", cellHardware.getActiveAlertCount());
        Serial.printf("Common Buzzer      : %s (Channel %u)\n",
                      cellHardware.getActiveAlertCount() > 0 ? "PULSING" : "SILENT",
                      BUZZER_LEDC_CHANNEL);
        Serial.println("------------------------------\n");
        return;
    }

    if (line.startsWith("DUE ")) {
        int cellNum = line.substring(4).toInt();
        if (cellNum >= 1 && cellNum <= (int)cellHardware.getCellCount()) {
            Serial.printf("[COMMAND] Triggering MEDICINE_DUE for Cell %d\n", cellNum);
            cellHardware.triggerMedicineDue(cellNum - 1);
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, cellHardware.getCellCount());
        }
    } else if (line.startsWith("MISSED ")) {
        int cellNum = line.substring(7).toInt();
        if (cellNum >= 1 && cellNum <= (int)cellHardware.getCellCount()) {
            Serial.printf("[COMMAND] Triggering MISSED_DOSE for Cell %d\n", cellNum);
            cellHardware.triggerMissedDose(cellNum - 1);
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, cellHardware.getCellCount());
        }
    } else if (line.startsWith("CONFIRM ")) {
        int cellNum = line.substring(8).toInt();
        if (cellNum >= 1 && cellNum <= (int)cellHardware.getCellCount()) {
            Serial.printf("[COMMAND] Simulating CONFIRM for Cell %d\n", cellNum);
            cellHardware.triggerDoseConfirmed(cellNum - 1);
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, cellHardware.getCellCount());
        }
    } else if (line.startsWith("RESET ")) {
        int cellNum = line.substring(6).toInt();
        if (cellNum >= 1 && cellNum <= (int)cellHardware.getCellCount()) {
            Serial.printf("[COMMAND] Resetting Cell %d to IDLE\n", cellNum);
            cellHardware.triggerReset(cellNum - 1);
        } else {
            Serial.printf("Invalid cell number: %d (Available: 1 to %u)\n", cellNum, cellHardware.getCellCount());
        }
    } else if (line == "RESET") {
        Serial.println("[COMMAND] Resetting all 4 cells to IDLE");
        cellHardware.triggerResetAll();
    } else {
        Serial.printf("Unknown command: '%s'. Try DUE 1, CONFIRM 1, MISSED 1, RESET 1, STATUS\n", line.c_str());
    }
}