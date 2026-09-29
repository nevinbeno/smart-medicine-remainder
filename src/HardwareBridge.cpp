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

void initHardwareBridge(BridgeConfirmCallback onConfirm, uint8_t activeCellCount) {
    g_bridgeConfirmCallback = onConfirm;
    cellHardware.setConfirmationCallback(internalConfirmCallback);
    cellHardware.setStateChangedCallback(internalStateChangedCallback);
    cellHardware.setCellCount(activeCellCount);
    cellHardware.begin();
}

bool setHardwareCellCount(uint8_t cellCount) {
    return cellHardware.setCellCount(cellCount);
}

bool setHardwareGrid(uint8_t rows, uint8_t cols) {
    return cellHardware.setGrid(rows, cols);
}

uint8_t getHardwareGridRows() {
    return cellHardware.getGridRows();
}

uint8_t getHardwareGridCols() {
    return cellHardware.getGridCols();
}

int8_t getHardwareCellIndex(uint8_t row, uint8_t col) {
    return cellHardware.gridCoordsToIndex(row, col);
}

bool getHardwareCellGridCoords(uint8_t cellIndex, uint8_t& outRow, uint8_t& outCol) {
    return cellHardware.indexToGridCoords(cellIndex, outRow, outCol);
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

bool triggerHardwareMedicineDueGrid(uint8_t row, uint8_t col) {
    return cellHardware.triggerMedicineDueGrid(row, col);
}

bool triggerHardwareMissedDoseGrid(uint8_t row, uint8_t col) {
    return cellHardware.triggerMissedDoseGrid(row, col);
}

bool triggerHardwareDoseConfirmedGrid(uint8_t row, uint8_t col) {
    return cellHardware.triggerDoseConfirmedGrid(row, col);
}

bool triggerHardwareResetGrid(uint8_t row, uint8_t col) {
    return cellHardware.triggerResetGrid(row, col);
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
    uint8_t rows = cellHardware.getGridRows();
    uint8_t cols = cellHardware.getGridCols();
    uint8_t total = cellHardware.getCellCount();

    Serial.println("\n=======================================================");
    Serial.printf(" HARDWARE STATUS: Matrix Grid [%ux%u] = %u Active Cells\n", rows, cols, total);
    Serial.printf(" Physical Direct GPIOs : Cells 1..%u\n",
                  HW_DIRECT_PIN_CELLS < total ? HW_DIRECT_PIN_CELLS : total);
    if (total > HW_DIRECT_PIN_CELLS) {
        Serial.printf(" Virtual/Expander GPIOs: Cells %u..%u\n", HW_DIRECT_PIN_CELLS + 1, total);
    }
    Serial.printf(" Active Alerts         : %u\n", cellHardware.getActiveAlertCount());
    Serial.printf(" Common Buzzer         : %s (Channel %u)\n",
                  cellHardware.getActiveAlertCount() > 0 ? "PULSING" : "SILENT",
                  BUZZER_LEDC_CHANNEL);
    Serial.println("-------------------------------------------------------");
    Serial.println(" Compartment Grid Matrix [Cell (Row,Col): State | Servo Angle]:");

    for (uint8_t r = 1; r <= rows; r++) {
        Serial.printf("  Row %u: ", r);
        for (uint8_t c = 1; c <= cols; c++) {
            int8_t idx = cellHardware.gridCoordsToIndex(r, c);
            if (idx >= 0) {
                CellHardware* cell = cellHardware.getCell((uint8_t)idx);
                const char* st = cellHardware.isCellAlerting((uint8_t)idx) ? "DUE" :
                                 (cellHardware.isCellWarning((uint8_t)idx) ? "MISSED" :
                                 (cellHardware.isCellConfirmed((uint8_t)idx) ? "CONFIRM" : "IDLE"));
                uint8_t angle = (cell ? cell->getServo().getCurrentAngle() : 0);
                Serial.printf("[C%02u: %-7s %2u°] ", idx + 1, st, angle);
            }
        }
        Serial.println();
    }
    Serial.println("=======================================================\n");
}
