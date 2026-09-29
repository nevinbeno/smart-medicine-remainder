#ifndef CELL_ACTUATORS_H
#define CELL_ACTUATORS_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "HardwareConfig.h"
#include "CellHardwareTypes.h"

/**
 * @file CellActuators.h
 * @brief Consolidated peripheral drivers for 4-cell hardware:
 *        - DebouncedButton : Software debounced push-button reader
 *        - RgbLed          : Discrete 3-channel RGB LED driver (Red, Green, Yellow)
 *        - ServoController : Dedicated compartment servo motor actuator
 *        - BuzzerController: Multi-cell arbitrated alert buzzer driver
 */

// ============================================================================
// 1. DEBOUNCED PUSH BUTTON DRIVER
// ============================================================================

/**
 * @class DebouncedButton
 * @brief Non-blocking software debouncer with single-edge trigger detection.
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
    DebouncedButton(int8_t pin = -1, bool activeLow = true, uint32_t debounceDelayMs = 50);
    void begin();
    bool wasPressed();
    bool isHeld() const;
    int8_t getPin() const { return _pin; }
};

// ============================================================================
// 2. RGB LED DRIVER
// ============================================================================

/**
 * @class RgbLed
 * @brief Driver for a discrete 3-channel RGB LED (Common Cathode / Common Anode).
 */
class RgbLed {
private:
    int8_t _pinR;
    int8_t _pinG;
    int8_t _pinB;
    bool _commonAnode;
    CellColor _currentColor;

    void applyChannels(bool r, bool g, bool b);

public:
    RgbLed(int8_t pinR = -1, int8_t pinG = -1, int8_t pinB = -1, bool commonAnode = false);
    void begin();
    void setColor(CellColor color);
    void setRed();
    void setGreen();
    void setYellow();
    void setBlue();
    void setOff();
    CellColor getCurrentColor() const;
};

// ============================================================================
// 3. SERVO MOTOR CONTROLLER
// ============================================================================

/**
 * @class ServoController
 * @brief Dedicated servo actuator driver for an individual medicine compartment.
 */
class ServoController {
private:
    int8_t _pin;
    Servo _servo;
    uint8_t _currentAngle;
    bool _isAttached;
    bool _isOpen;

public:
    ServoController(int8_t pin = -1);
    void begin();
    void writeAngle(uint8_t angle);
    void open(uint8_t angle = 90);
    void close(uint8_t angle = 0);
    bool isOpen() const { return _isOpen; }
    uint8_t getCurrentAngle() const { return _currentAngle; }
    void detach();
};

// ============================================================================
// 4. COMMON BUZZER CONTROLLER
// ============================================================================

/**
 * @class BuzzerController
 * @brief Common buzzer alert controller with multi-cell alert arbitration.
 */
class BuzzerController {
public:
    enum BuzzerMode {
        MODE_SILENT = 0,
        MODE_INTERMITTENT,
        MODE_CONTINUOUS
    };

private:
    int8_t _pin;
    uint8_t _channel;
    uint32_t _frequencyHz;
    uint32_t _onTimeMs;
    uint32_t _offTimeMs;

    BuzzerMode _mode;
    uint8_t _activeAlertingCount;
    bool _isToneActive;
    uint32_t _lastToggleTime;
    bool _initialized;

    void toneOn();
    void toneOff();

public:
    BuzzerController(int8_t pin = PIN_COMMON_BUZZER,
                     uint8_t channel = BUZZER_LEDC_CHANNEL,
                     uint32_t frequencyHz = BUZZER_FREQ_HZ,
                     uint32_t onTimeMs = BUZZER_PULSE_ON_MS,
                     uint32_t offTimeMs = BUZZER_PULSE_OFF_MS);

    void begin();
    void update();
    void setAlertingCount(uint8_t count);
    void silence();
    void setMode(BuzzerMode mode);
    bool isActive() const { return _activeAlertingCount > 0; }
    uint8_t getAlertingCount() const { return _activeAlertingCount; }
};

#endif // CELL_ACTUATORS_H
