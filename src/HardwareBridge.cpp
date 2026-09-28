#include "HardwareBridge.h"
#include "CellHardwareEvents.h"

// Global instance of the 4-cell hardware controller (Role 4)
CellHardwareController cellHardware(ACTIVE_CELL_COUNT);

static BridgeConfirmCallback g_bridgeConfirmCallback = nullptr;

/**
 * @brief Outbound Event Handler: DOSE_CONFIRMED
 *        Invoked when physical confirm button on a cell is debounced and pressed.
 */
static void internalConfirmCallback(uint8_t cellId) {
    Serial.printf("\n>>> [HARDWARE EVENT] Physical DOSE_CONFIRMED button pressed on Cell %u!\n", cellId + 1);
    Serial.printf("    Hardware Action: Cell %u RGB is GREEN for 1s. Servo %u is CLOSED.\n\n", cellId + 1, cellId + 1);
    if (g_bridgeConfirmCallback) {
        g_bridgeConfirmCallback(cellId);
    }
}

/**
 * @brief State Change Observer Callback
 *        Invoked whenever any cell undergoes a state machine transition.
 */
static void internalStateChangedCallback(uint8_t cellId, CellState oldState, CellState newState) {
    Serial.printf("[STATE CHANGE] Cell %u: %s -> %s\n",
                  cellId + 1, cellStateToString(oldState), cellStateToString(newState));
}

void initHardwareBridge(BridgeConfirmCallback onConfirm) {
    g_bridgeConfirmCallback = onConfirm;
    cellHardware.setConfirmationCallback(internalConfirmCallback);
    cellHardware.setStateChangedCallback(internalStateChangedCallback);
    cellHardware.begin();
}

void updateHardwareBridge() {
    cellHardware.update();
}

bool triggerHardwareMedicineDue(uint8_t cellIndex) {
    return cellHardware.triggerMedicineDue(cellIndex);
}

bool triggerHardwareMissedDose(uint8_t cellIndex) {
    return cellHardware.triggerMissedDose(cellIndex);
}

bool triggerHardwareDoseConfirmed(uint8_t cellIndex) {
    return cellHardware.triggerDoseConfirmed(cellIndex);
}

bool triggerHardwareReset(uint8_t cellIndex) {
    return cellHardware.triggerReset(cellIndex);
}

void triggerHardwareResetAll() {
    cellHardware.triggerResetAll();
}

uint8_t getHardwareCellCount() {
    return cellHardware.getCellCount();
}

uint8_t getHardwareActiveAlertCount() {
    return cellHardware.getActiveAlertCount();
}

const char* getHardwareCellStateString(uint8_t cellIndex) {
    return cellStateToString(cellHardware.getCellState(cellIndex));
}

bool isHardwareCellServoOpen(uint8_t cellIndex) {
    CellHardware* c = cellHardware.getCell(cellIndex);
    return (c && c->getServo().isOpen());
}

uint8_t getHardwareCellServoAngle(uint8_t cellIndex) {
    CellHardware* c = cellHardware.getCell(cellIndex);
    return (c ? c->getServo().getCurrentAngle() : 0);
}

void printHardwareStatus() {
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
}
