#ifndef DEBOUNCED_BUTTON_H
#define DEBOUNCED_BUTTON_H

#include <Arduino.h>

/**
 * @file DebouncedButton.h
 * @brief Non-blocking software debouncer with single-edge trigger detection.
 *
 * Mechanical push buttons exhibit contact chatter/bounce for 5-30ms upon transition.
 * This class ensures that a physical press registers exactly ONE logical event and
 * prevents re-triggering while held down.
 */
class DebouncedButton {
private:
    int8_t _pin;
    bool _activeLow;
    uint32_t _debounceDelayMs;

    bool _lastRawReading;
    bool _stableState;
    uint32_t _lastDebounceTime;
    bool _eventConsumed;

public:
    /**
     * @brief Construct a debounced button reader.
     * @param pin GPIO pin connected to button.
     * @param activeLow True if button connects pin to GND when pressed (using INPUT_PULLUP).
     * @param debounceDelayMs Milliseconds required for state stability (default 50ms).
     */
    DebouncedButton(int8_t pin = -1, bool activeLow = true, uint32_t debounceDelayMs = 50);

    /**
     * @brief Configure GPIO pin mode (INPUT_PULLUP if activeLow).
     */
    void begin();

    /**
     * @brief Non-blocking state update. Call every loop cycle.
     * @return True EXACTLY ONCE on transition from released to pressed (leading edge).
     */
    bool wasPressed();

    /**
     * @brief Query current debounced steady state.
     * @return True if button is currently held down.
     */
    bool isHeld() const;

    /**
     * @brief Get the configured pin.
     */
    int8_t getPin() const { return _pin; }
};

#endif // DEBOUNCED_BUTTON_H
