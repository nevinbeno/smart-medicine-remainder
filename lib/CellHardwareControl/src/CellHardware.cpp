#include "CellHardware.h"

CellHardware::CellHardware(uint8_t cellId, const CellPinConfig& config, uint32_t greenDurationMs)
    : _cellId(cellId),
      _config(config),
      _state(CELL_STATE_IDLE),
      _rgb(config.pinRed, config.pinGreen, config.pinBlue, false), // common cathode
      _button(config.pinButton, true, DEBOUNCE_DELAY_MS),
      _servo(config.pinServo),
      _confirmStartTime(0),
      _greenDurationMs(greenDurationMs),
      _virtualPressPending(false) {}

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
    if (isVirtual()) {
        logVirtualTransition(newState);
    }
}

bool CellHardware::isVirtual() const {
    return (_config.pinRed < 0 && _config.pinGreen < 0 &&
            _config.pinButton < 0 && _config.pinServo < 0);
}

void CellHardware::logVirtualTransition(CellState newState) const {
    const char* color = "OFF";
    switch (newState) {
        case CELL_STATE_ALERTING:  color = "RED (DUE)"; break;
        case CELL_STATE_CONFIRMED: color = "GREEN (CONFIRMED 1s)"; break;
        case CELL_STATE_WARNING:   color = "YELLOW (MISSED)"; break;
        case CELL_STATE_IDLE:
        default:                   color = "OFF (IDLE)"; break;
    }
    // Servo angle mirrors physical behavior: OPEN=90 on DUE, CLOSED=0 otherwise.
    uint8_t angle = (newState == CELL_STATE_ALERTING) ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE;
    Serial.printf("[VIRTUAL] Cell %u -> LED %s | Servo %u deg (%s) | use PRESS %u to confirm\n",
                  _cellId + 1, color, angle, (angle > 0 ? "OPEN" : "CLOSED"), _cellId + 1);
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

    // 2. Virtual button injection (serial PRESS <id>): consume once.
    if (_virtualPressPending) {
        _virtualPressPending = false;
        return true;
    }

    // 3. Poll debounced button
    return _button.wasPressed();
}
