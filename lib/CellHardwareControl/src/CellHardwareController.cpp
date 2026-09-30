#include "CellHardwareController.h"

CellHardwareController::CellHardwareController(uint8_t cellCount,
                                               const CellPinConfig* pinConfigs,
                                               int8_t pinBuzzer)
    : _cellCount(cellCount > HW_MAX_CELLS ? HW_MAX_CELLS : (cellCount == 0 ? 1 : cellCount)),
      _gridRows(2),
      _gridCols(2),
      _buzzer(pinBuzzer, BUZZER_LEDC_CHANNEL, BUZZER_FREQ_HZ, BUZZER_PULSE_ON_MS, BUZZER_PULSE_OFF_MS),
      _doseConfirmedCallback(nullptr),
      _stateChangedCallback(nullptr) {

    // Auto-compute default grid geometry
    if (_cellCount == 4) { _gridRows = 2; _gridCols = 2; }
    else if (_cellCount == 9) { _gridRows = 3; _gridCols = 3; }
    else if (_cellCount == 16) { _gridRows = 4; _gridCols = 4; }
    else if (_cellCount == 64) { _gridRows = 8; _gridCols = 8; }
    else { _gridRows = 1; _gridCols = _cellCount; }

    for (uint8_t i = 0; i < HW_MAX_CELLS; i++) {
        CellPinConfig cfg = { -1, -1, -1, -1, -1 };
        if (i < HW_DIRECT_PIN_CELLS && pinConfigs != nullptr) {
            cfg = pinConfigs[i];
        }
        _cells[i] = new CellHardware(i, cfg, CONFIRM_GREEN_DURATION_MS);
    }
}

bool CellHardwareController::setGrid(uint8_t rows, uint8_t cols) {
    if (rows == 0 || cols == 0) return false;
    uint16_t total = (uint16_t)rows * (uint16_t)cols;
    if (total == 0 || total > HW_MAX_CELLS) return false;

    _gridRows = rows;
    _gridCols = cols;
    return setCellCount((uint8_t)total);
}

int8_t CellHardwareController::gridCoordsToIndex(uint8_t row, uint8_t col) const {
    if (row < 1 || row > _gridRows || col < 1 || col > _gridCols) {
        return -1;
    }
    uint8_t idx = (row - 1) * _gridCols + (col - 1);
    if (idx >= _cellCount) return -1;
    return (int8_t)idx;
}

bool CellHardwareController::indexToGridCoords(uint8_t cellIndex, uint8_t& outRow, uint8_t& outCol) const {
    if (cellIndex >= _cellCount) return false;
    outRow = (cellIndex / _gridCols) + 1;
    outCol = (cellIndex % _gridCols) + 1;
    return true;
}

bool CellHardwareController::setCellCount(uint8_t count) {
    if (count == 0 || count > HW_MAX_CELLS) {
        return false;
    }
    if (count == _cellCount) {
        return true;
    }

    if (count < _cellCount) {
        // Deactivating cells: reset them so LEDs are OFF and servos CLOSED
        for (uint8_t i = count; i < _cellCount; i++) {
            if (_cells[i] != nullptr) {
                _cells[i]->triggerReset();
            }
        }
    } else {
        // Activating cells: initialize GPIOs and servos
        for (uint8_t i = _cellCount; i < count; i++) {
            if (_cells[i] != nullptr) {
                _cells[i]->begin();
            }
        }
    }

    _cellCount = count;
    // Auto-update grid dimensions if not matching
    if ((uint16_t)_gridRows * (uint16_t)_gridCols != count) {
        if (count == 4) { _gridRows = 2; _gridCols = 2; }
        else if (count == 9) { _gridRows = 3; _gridCols = 3; }
        else if (count == 16) { _gridRows = 4; _gridCols = 4; }
        else if (count == 64) { _gridRows = 8; _gridCols = 8; }
        else { _gridRows = 1; _gridCols = count; }
    }

    syncBuzzer();
    return true;
}

CellHardwareController::~CellHardwareController() {
    for (uint8_t i = 0; i < HW_MAX_CELLS; i++) {
        if (_cells[i] != nullptr) {
            delete _cells[i];
            _cells[i] = nullptr;
        }
    }
}

void CellHardwareController::begin() {
    // 1. Initialize common alert buzzer
    _buzzer.begin();

    // 2. Initialize each independent cell (RGB LED, Debounced Button, Dedicated Servo)
    for (uint8_t i = 0; i < HW_MAX_CELLS; i++) {
        if (_cells[i] != nullptr) {
            _cells[i]->begin();
        }
    }

    // 3. For any cells beyond _cellCount, ensure they are in IDLE state
    for (uint8_t i = _cellCount; i < HW_MAX_CELLS; i++) {
        if (_cells[i] != nullptr) {
            _cells[i]->triggerReset();
        }
    }

    // 4. Initial sync
    syncBuzzer();
}

uint8_t CellHardwareController::calculateAlertingCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cells[i] != nullptr && _cells[i]->isAlerting()) {
            count++;
        }
    }
    return count;
}

void CellHardwareController::syncBuzzer() {
    uint8_t alertingCount = calculateAlertingCount();
    // Common buzzer pulses while at least ONE cell is in ALERTING state
    _buzzer.setAlertingCount(alertingCount);
}

void CellHardwareController::update() {
    // 1. Poll each of the 4 cells for button presses and green-light auto-timeout
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cells[i] == nullptr) continue;

        CellState stateBefore = _cells[i]->getState();
        bool buttonPressed = _cells[i]->update();
        CellState stateAfter = _cells[i]->getState();

        // Check if green-light 1-second timer expired (CONFIRMED -> IDLE)
        if (stateBefore == CELL_STATE_CONFIRMED && stateAfter == CELL_STATE_IDLE) {
            if (_stateChangedCallback != nullptr) {
                _stateChangedCallback(i, stateBefore, stateAfter);
            }
        }

        // Handle physical button press for this cell
        if (buttonPressed) {
            // Only confirm if this cell was actively ALERTING (due) or WARNING (missed)
            if (stateBefore == CELL_STATE_ALERTING || stateBefore == CELL_STATE_WARNING) {
                _cells[i]->triggerConfirm(); // Transitions to CONFIRMED: LED GREEN for 1s, THIS cell's servo closes!

                // Re-evaluate common buzzer (if other cells are still alerting, buzzer keeps sounding!)
                syncBuzzer();

                // Fire outbound DOSE_CONFIRMED event for Role 5 (Dose Manager)
                if (_doseConfirmedCallback != nullptr) {
                    _doseConfirmedCallback(i);
                }

                // Fire state change callback for UI / Logger
                if (_stateChangedCallback != nullptr) {
                    _stateChangedCallback(i, stateBefore, CELL_STATE_CONFIRMED);
                }
            }
            // If button pressed while IDLE, safely ignore.
        }
    }

    // 2. Common buzzer cadence update (non-blocking pulse generation)
    _buzzer.update();
}

bool CellHardwareController::triggerMedicineDue(uint8_t cellId) {
    if (cellId >= _cellCount || _cells[cellId] == nullptr) return false;

    CellState oldState = _cells[cellId]->getState();
    _cells[cellId]->triggerMedicineDue(); // Cell's LED turns RED, Cell's servo OPENS (90 deg)
    syncBuzzer();

    if (_stateChangedCallback != nullptr && oldState != CELL_STATE_ALERTING) {
        _stateChangedCallback(cellId, oldState, CELL_STATE_ALERTING);
    }
    return true;
}

bool CellHardwareController::triggerMissedDose(uint8_t cellId) {
    if (cellId >= _cellCount || _cells[cellId] == nullptr) return false;

    CellState oldState = _cells[cellId]->getState();
    _cells[cellId]->triggerMissedDose(); // Cell's LED turns YELLOW, Cell's servo CLOSES (0 deg)
    syncBuzzer();

    if (_stateChangedCallback != nullptr && oldState != CELL_STATE_WARNING) {
        _stateChangedCallback(cellId, oldState, CELL_STATE_WARNING);
    }
    return true;
}

bool CellHardwareController::triggerDoseConfirmed(uint8_t cellId) {
    if (cellId >= _cellCount || _cells[cellId] == nullptr) return false;

    CellState oldState = _cells[cellId]->getState();
    _cells[cellId]->triggerConfirm(); // Cell's LED turns GREEN for 1s, Cell's servo CLOSES
    syncBuzzer();

    if (_doseConfirmedCallback != nullptr) {
        _doseConfirmedCallback(cellId);
    }
    if (_stateChangedCallback != nullptr && oldState != CELL_STATE_CONFIRMED) {
        _stateChangedCallback(cellId, oldState, CELL_STATE_CONFIRMED);
    }
    return true;
}

bool CellHardwareController::triggerReset(uint8_t cellId) {
    if (cellId >= _cellCount || _cells[cellId] == nullptr) return false;

    CellState oldState = _cells[cellId]->getState();
    _cells[cellId]->triggerReset();
    syncBuzzer();

    if (_stateChangedCallback != nullptr && oldState != CELL_STATE_IDLE) {
        _stateChangedCallback(cellId, oldState, CELL_STATE_IDLE);
    }
    return true;
}

void CellHardwareController::triggerResetAll() {
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cells[i] != nullptr) {
            _cells[i]->triggerReset();
        }
    }
    syncBuzzer();
}

void CellHardwareController::setConfirmationCallback(DoseConfirmedCallback callback) {
    _doseConfirmedCallback = callback;
}

void CellHardwareController::setStateChangedCallback(CellStateChangedCallback callback) {
    _stateChangedCallback = callback;
}

CellState CellHardwareController::getCellState(uint8_t cellId) const {
    if (cellId < _cellCount && _cells[cellId] != nullptr) {
        return _cells[cellId]->getState();
    }
    return CELL_STATE_IDLE;
}

bool CellHardwareController::isCellVirtual(uint8_t cellId) const {
    if (cellId < _cellCount && _cells[cellId] != nullptr) {
        return _cells[cellId]->isVirtual();
    }
    return cellId >= HW_DIRECT_PIN_CELLS;
}

bool CellHardwareController::injectVirtualButtonPress(uint8_t cellId) {
    if (cellId >= _cellCount || _cells[cellId] == nullptr) return false;
    _cells[cellId]->injectButtonPress();
    return true;
}

bool CellHardwareController::injectVirtualButtonPressGrid(uint8_t row, uint8_t col) {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return false;
    return injectVirtualButtonPress((uint8_t)idx);
}

bool CellHardwareController::isCellAlerting(uint8_t cellId) const {
    return getCellState(cellId) == CELL_STATE_ALERTING;
}

bool CellHardwareController::isCellWarning(uint8_t cellId) const {
    return getCellState(cellId) == CELL_STATE_WARNING;
}

bool CellHardwareController::isCellConfirmed(uint8_t cellId) const {
    return getCellState(cellId) == CELL_STATE_CONFIRMED;
}

bool CellHardwareController::isCellIdle(uint8_t cellId) const {
    return getCellState(cellId) == CELL_STATE_IDLE;
}

CellHardware* CellHardwareController::getCell(uint8_t cellId) {
    if (cellId < _cellCount) {
        return _cells[cellId];
    }
    return nullptr;
}

bool CellHardwareController::triggerMedicineDueGrid(uint8_t row, uint8_t col) {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return false;
    return triggerMedicineDue((uint8_t)idx);
}

bool CellHardwareController::triggerMissedDoseGrid(uint8_t row, uint8_t col) {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return false;
    return triggerMissedDose((uint8_t)idx);
}

bool CellHardwareController::triggerDoseConfirmedGrid(uint8_t row, uint8_t col) {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return false;
    return triggerDoseConfirmed((uint8_t)idx);
}

bool CellHardwareController::triggerResetGrid(uint8_t row, uint8_t col) {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return false;
    return triggerReset((uint8_t)idx);
}

CellState CellHardwareController::getCellStateGrid(uint8_t row, uint8_t col) const {
    int8_t idx = gridCoordsToIndex(row, col);
    if (idx < 0) return CELL_STATE_IDLE;
    return getCellState((uint8_t)idx);
}
