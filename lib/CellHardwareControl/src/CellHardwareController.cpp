#include "CellHardwareController.h"

CellHardwareController::CellHardwareController(uint8_t cellCount,
                                               const CellPinConfig* pinConfigs,
                                               int8_t pinBuzzer)
    : _cellCount(cellCount > MAX_CELLS ? MAX_CELLS : cellCount),
      _buzzer(pinBuzzer, BUZZER_LEDC_CHANNEL, BUZZER_FREQ_HZ, BUZZER_PULSE_ON_MS, BUZZER_PULSE_OFF_MS),
      _doseConfirmedCallback(nullptr),
      _stateChangedCallback(nullptr) {

    for (uint8_t i = 0; i < MAX_CELLS; i++) {
        if (i < _cellCount && pinConfigs != nullptr) {
            _cells[i] = new CellHardware(i, pinConfigs[i], CONFIRM_GREEN_DURATION_MS);
        } else {
            _cells[i] = nullptr;
        }
    }
}

CellHardwareController::~CellHardwareController() {
    for (uint8_t i = 0; i < MAX_CELLS; i++) {
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
    for (uint8_t i = 0; i < _cellCount; i++) {
        if (_cells[i] != nullptr) {
            _cells[i]->begin();
        }
    }

    // 3. Initial sync
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
