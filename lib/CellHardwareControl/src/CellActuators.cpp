#include "CellActuators.h"

// ============================================================================
// 1. DEBOUNCED BUTTON IMPLEMENTATION
// ============================================================================

DebouncedButton::DebouncedButton(int8_t pin, bool activeLow, uint32_t debounceDelayMs)
    : _pin(pin),
      _activeLow(activeLow),
      _debounceDelayMs(debounceDelayMs),
      _lastRawReading(activeLow ? HIGH : LOW),
      _stableState(activeLow ? HIGH : LOW),
      _lastDebounceTime(0),
      _eventConsumed(false) {}

void DebouncedButton::begin() {
    if (_pin >= 0) {
        if (_activeLow) {
            pinMode(_pin, INPUT_PULLUP);
            _stableState = HIGH;
            _lastRawReading = HIGH;
        } else {
            pinMode(_pin, INPUT);
            _stableState = LOW;
            _lastRawReading = LOW;
        }
    }
}

bool DebouncedButton::wasPressed() {
    if (_pin < 0) return false;

    bool rawState = digitalRead(_pin);

    // If signal fluctuated due to contact bounce or noise, restart stability timer
    if (rawState != _lastRawReading) {
        _lastDebounceTime = millis();
        _lastRawReading = rawState;
    }

    // Check if the signal has remained stable for longer than debounce threshold
    if ((millis() - _lastDebounceTime) >= _debounceDelayMs) {
        if (rawState != _stableState) {
            _stableState = rawState;
            bool isNowPressed = _activeLow ? (_stableState == LOW) : (_stableState == HIGH);

            if (isNowPressed) {
                if (!_eventConsumed) {
                    _eventConsumed = true; // Latch event to prevent repeated firings while held
                    return true;          // Trigger single genuine press event
                }
            } else {
                _eventConsumed = false;
            }
        }
    }

    return false;
}

bool DebouncedButton::isHeld() const {
    if (_pin < 0) return false;
    return _activeLow ? (_stableState == LOW) : (_stableState == HIGH);
}

// ============================================================================
// 2. RGB LED IMPLEMENTATION
// ============================================================================

RgbLed::RgbLed(int8_t pinR, int8_t pinG, int8_t pinB, bool commonAnode)
    : _pinR(pinR),
      _pinG(pinG),
      _pinB(pinB),
      _commonAnode(commonAnode),
      _currentColor(COLOR_OFF) {}

void RgbLed::begin() {
    if (_pinR >= 0) pinMode(_pinR, OUTPUT);
    if (_pinG >= 0) pinMode(_pinG, OUTPUT);
    if (_pinB >= 0) pinMode(_pinB, OUTPUT);
    setOff();
}

void RgbLed::applyChannels(bool r, bool g, bool b) {
    uint8_t levelR = _commonAnode ? (r ? LOW : HIGH) : (r ? HIGH : LOW);
    uint8_t levelG = _commonAnode ? (g ? LOW : HIGH) : (g ? HIGH : LOW);
    uint8_t levelB = _commonAnode ? (b ? LOW : HIGH) : (b ? HIGH : LOW);

    if (_pinR >= 0) digitalWrite(_pinR, levelR);
    if (_pinG >= 0) digitalWrite(_pinG, levelG);
    if (_pinB >= 0) digitalWrite(_pinB, levelB);
}

void RgbLed::setColor(CellColor color) {
    _currentColor = color;
    switch (color) {
        case COLOR_RED:
            applyChannels(true, false, false);
            break;
        case COLOR_GREEN:
            applyChannels(false, true, false);
            break;
        case COLOR_YELLOW:
            // Additive color mixing: Red + Green = Yellow
            applyChannels(true, true, false);
            break;
        case COLOR_BLUE:
            applyChannels(false, false, true);
            break;
        case COLOR_WHITE:
            applyChannels(true, true, true);
            break;
        case COLOR_OFF:
        default:
            applyChannels(false, false, false);
            break;
    }
}

void RgbLed::setRed()    { setColor(COLOR_RED); }
void RgbLed::setGreen()  { setColor(COLOR_GREEN); }
void RgbLed::setYellow() { setColor(COLOR_YELLOW); }
void RgbLed::setBlue()   { setColor(COLOR_BLUE); }
void RgbLed::setOff()    { setColor(COLOR_OFF); }

CellColor RgbLed::getCurrentColor() const {
    return _currentColor;
}

// ============================================================================
// 3. SERVO CONTROLLER IMPLEMENTATION
// ============================================================================

ServoController::ServoController(int8_t pin)
    : _pin(pin),
      _currentAngle(0),
      _isAttached(false),
      _isOpen(false) {}

void ServoController::begin() {
    if (_pin >= 0 && !_isAttached) {
        // Pre-allocate hardware timers 1, 2, 3 for ESP32Servo
        // Leaving Timer 0 exclusively for tone() buzzer alert generation!
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);

        _servo.setPeriodHertz(50);        // Standard 50Hz servo frequency
        _servo.attach(_pin, 500, 2400);   // Standard 500us to 2400us limits for 0-180 deg
        _isAttached = true;
        close(0);
    }
}

void ServoController::writeAngle(uint8_t angle) {
    if (!_isAttached && _pin >= 0) {
        begin();
    }
    if (_isAttached) {
        if (angle > 180) angle = 180;
        _currentAngle = angle;
        _servo.write(angle);
    }
}

void ServoController::open(uint8_t angle) {
    writeAngle(angle);
    _isOpen = true;
}

void ServoController::close(uint8_t angle) {
    writeAngle(angle);
    _isOpen = false;
}

void ServoController::detach() {
    if (_isAttached) {
        _servo.detach();
        _isAttached = false;
    }
}

// ============================================================================
// 4. BUZZER CONTROLLER IMPLEMENTATION
// ============================================================================

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
        _lastToggleTime = millis();
        toneOn();
    } else if (_activeAlertingCount == 0 && previousCount > 0) {
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
