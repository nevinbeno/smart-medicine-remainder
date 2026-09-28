#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "HardwareConfig.h"

/**
 * @file BuzzerController.h
 * @brief Common buzzer alert controller with multi-cell alert arbitration.
 *
 * Utilizes a dedicated LEDC channel (Channel 6 / Timer 3) to generate a clean 2500 Hz
 * square wave for the piezo buzzer without conflicting with servo timers.
 *
 * Arbitrates active alerts from all 4 cells: remains active as long as at least ONE
 * cell is in the ALERTING state.
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
    /**
     * @brief Construct Buzzer Controller.
     * @param pin GPIO pin connected to buzzer.
     * @param channel Dedicated LEDC channel (default BUZZER_LEDC_CHANNEL 6).
     * @param frequencyHz Resonant acoustic frequency (default 2500 Hz).
     * @param onTimeMs Intermittent pulse ON duration (default 300 ms).
     * @param offTimeMs Intermittent pulse OFF duration (default 300 ms).
     */
    BuzzerController(int8_t pin = PIN_COMMON_BUZZER,
                     uint8_t channel = BUZZER_LEDC_CHANNEL,
                     uint32_t frequencyHz = BUZZER_FREQ_HZ,
                     uint32_t onTimeMs = BUZZER_PULSE_ON_MS,
                     uint32_t offTimeMs = BUZZER_PULSE_OFF_MS);

    /**
     * @brief Configure LEDC peripheral and silence buzzer.
     */
    void begin();

    /**
     * @brief Non-blocking periodic update. Must be called in main loop.
     */
    void update();

    /**
     * @brief Update the count of currently alerting cells (arbitration engine).
     *        If count > 0: buzzer activates or continues cadence.
     *        If count == 0: buzzer silences immediately.
     * @param count Number of cells requiring alert across all 4 cells.
     */
    void setAlertingCount(uint8_t count);

    /**
     * @brief Force silence the buzzer.
     */
    void silence();

    /**
     * @brief Set alert mode (Intermittent vs Continuous).
     */
    void setMode(BuzzerMode mode);

    /**
     * @brief Check if buzzer is currently active.
     */
    bool isActive() const { return _activeAlertingCount > 0; }

    /**
     * @brief Query currently registered alerting cell count.
     */
    uint8_t getAlertingCount() const { return _activeAlertingCount; }
};

#endif // BUZZER_CONTROLLER_H
