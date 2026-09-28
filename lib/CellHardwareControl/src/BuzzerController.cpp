#include "BuzzerController.h"

BuzzerController::BuzzerController(int8_t pin, uint8_t channel, uint32_t frequencyHz, uint32_t onTimeMs, uint32_t offTimeMs)
    : _pin(pin),
      _channel(channel),
      _frequencyHz(frequencyHz),
      _onTimeMs(onTimeMs),
      _offTimeMs(offTimeMs),
      _mode(MODE_INTERMITTENT),
      _activeAlertingCount(0),
      _isToneActive(false),
      _lastToggleTime(0),
      _initialized(false) {}

void BuzzerController::begin() {
    if (_pin >= 0 && !_initialized) {
        pinMode(_pin, OUTPUT);
        noTone(_pin);
        _isToneActive = false;
        _initialized = true;
    }
}

void BuzzerController::toneOn() {
    if (_pin >= 0 && !_isToneActive) {
        if (!_initialized) begin();
        tone(_pin, _frequencyHz);
        _isToneActive = true;
    }
}

void BuzzerController::toneOff() {
    if (_pin >= 0 && _isToneActive) {
        if (!_initialized) begin();
        noTone(_pin);
        _isToneActive = false;
    }
}

void BuzzerController::setAlertingCount(uint8_t count) {
    uint8_t previousCount = _activeAlertingCount;
    _activeAlertingCount = count;

    if (_activeAlertingCount > 0 && previousCount == 0) {
        // Transition from 0 to >=1 alerting cells: start buzzer pulse immediately
        _lastToggleTime = millis();
        toneOn();
    } else if (_activeAlertingCount == 0 && previousCount > 0) {
        // All active cells cleared: silence buzzer immediately
        silence();
    }
}

void BuzzerController::silence() {
    toneOff();
    _activeAlertingCount = 0;
}

void BuzzerController::setMode(BuzzerMode mode) {
    _mode = mode;
}

void BuzzerController::update() {
    if (_pin < 0 || _activeAlertingCount == 0) {
        if (_isToneActive) {
            toneOff();
        }
        return;
    }

    uint32_t currentMillis = millis();

    switch (_mode) {
        case MODE_CONTINUOUS:
            toneOn();
            break;

        case MODE_INTERMITTENT:
            if (_isToneActive) {
                if (currentMillis - _lastToggleTime >= _onTimeMs) {
                    toneOff();
                    _lastToggleTime = currentMillis;
                }
            } else {
                if (currentMillis - _lastToggleTime >= _offTimeMs) {
                    toneOn();
                    _lastToggleTime = currentMillis;
                }
            }
            break;

        case MODE_SILENT:
        default:
            toneOff();
            break;
    }
}
