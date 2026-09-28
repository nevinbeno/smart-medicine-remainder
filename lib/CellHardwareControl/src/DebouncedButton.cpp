#include "DebouncedButton.h"

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
            // Note: On ESP32, GPIO 34-39 lack internal pull-up and require external pull-up.
            // For pins 0-33, INPUT_PULLUP activates the internal pull-up resistor.
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
        // If the debounced state has changed:
        if (rawState != _stableState) {
            _stableState = rawState;

            // Check if transition matches the active (pressed) level
            bool isNowPressed = _activeLow ? (_stableState == LOW) : (_stableState == HIGH);

            if (isNowPressed) {
                if (!_eventConsumed) {
                    _eventConsumed = true; // Latch event to prevent repeated firings while held
                    return true;          // Trigger single genuine press event
                }
            } else {
                // Button was released: clear latch so future presses can trigger
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
