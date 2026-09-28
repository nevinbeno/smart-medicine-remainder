#include "CellHardware.h"

CellHardware::CellHardware(uint8_t cellId, const CellPinConfig& config, uint32_t greenDurationMs)
    : _cellId(cellId),
      _config(config),
      _state(CELL_STATE_IDLE),
      _rgb(config.pinRed, config.pinGreen, config.pinBlue, false), // common cathode
      _button(config.pinButton, true, DEBOUNCE_DELAY_MS),
      _servo(config.pinServo),
      _confirmStartTime(0),
      _greenDurationMs(greenDurationMs) {}

void CellHardware::begin() {
    _rgb.begin();
    _button.begin();
    _servo.begin();
    _servo.close(SERVO_CLOSED_ANGLE);
    applyStateHardware(CELL_STATE_IDLE);
}

void CellHardware::applyStateHardware(CellState newState) {
    _state = newState;
    switch (_state) {
        case CELL_STATE_ALERTING:
            _rgb.setRed();
            _servo.open(SERVO_OPEN_ANGLE);
            break;

        case CELL_STATE_CONFIRMED:
            _rgb.setGreen();
            _confirmStartTime = millis();
            _servo.close(SERVO_CLOSED_ANGLE);
            break;

        case CELL_STATE_WARNING:
            _rgb.setYellow();
            _servo.close(SERVO_CLOSED_ANGLE);
            break;

        case CELL_STATE_IDLE:
        default:
            _rgb.setOff();
            _servo.close(SERVO_CLOSED_ANGLE);
            break;
    }
}

void CellHardware::triggerMedicineDue() {
    applyStateHardware(CELL_STATE_ALERTING);
}

void CellHardware::triggerMissedDose() {
    applyStateHardware(CELL_STATE_WARNING);
}

void CellHardware::triggerConfirm() {
    applyStateHardware(CELL_STATE_CONFIRMED);
}

void CellHardware::triggerReset() {
    applyStateHardware(CELL_STATE_IDLE);
}

bool CellHardware::update() {
    // 1. Auto-reset timer for the 1-second GREEN confirmation state
    if (_state == CELL_STATE_CONFIRMED) {
        if (millis() - _confirmStartTime >= _greenDurationMs) {
            applyStateHardware(CELL_STATE_IDLE);
        }
    }

    // 2. Poll debounced button
    return _button.wasPressed();
}
