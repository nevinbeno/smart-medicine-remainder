#ifndef CELL_HARDWARE_H
#define CELL_HARDWARE_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "CellHardwareTypes.h"
#include "RgbLed.h"
#include "DebouncedButton.h"
#include "ServoController.h"

/**
 * @file CellHardware.h
 * @brief Encapsulation of a single medicine compartment cell's hardware and state machine.
 *
 * Each cell independently owns and manages its dedicated:
 * - 3-channel RGB LED (Red, Green, Yellow, Off)
 * - Confirmation Push Button (with software debounce)
 * - Dedicated Compartment Servo (0 degrees closed, 90 degrees open)
 * - Local State Machine (IDLE, ALERTING, CONFIRMED, WARNING)
 */
class CellHardware {
private:
    uint8_t _cellId;
    CellPinConfig _config;
    CellState _state;

    RgbLed _rgb;
    DebouncedButton _button;
    ServoController _servo;

    uint32_t _confirmStartTime;
    uint32_t _greenDurationMs;

    void applyStateHardware(CellState newState);

public:
    /**
     * @brief Construct a CellHardware instance with dedicated peripherals.
     * @param cellId Numeric identifier for this cell (0-indexed).
     * @param config Pin configuration struct for this cell.
     * @param greenDurationMs Duration in ms for confirmation GREEN LED (default 1000ms).
     */
    CellHardware(uint8_t cellId = 0,
                 const CellPinConfig& config = { -1, -1, -1, -1, -1 },
                 uint32_t greenDurationMs = CONFIRM_GREEN_DURATION_MS);

    /**
     * @brief Initialize GPIOs, debouncer, LED, and servo for this cell.
     */
    void begin();

    /**
     * @brief Periodic update. Handles 1-second green timer auto-reset and button reads.
     * @return True if confirmation button was pressed on this cycle.
     */
    bool update();

    /**
     * @brief Hardware response to MEDICINE_DUE event.
     *        RGB LED -> RED, Dedicated Servo -> OPEN (90 deg).
     */
    void triggerMedicineDue();

    /**
     * @brief Hardware response to MISSED_DOSE event.
     *        RGB LED -> YELLOW, Dedicated Servo -> CLOSED (0 deg).
     */
    void triggerMissedDose();

    /**
     * @brief Hardware response to DOSE_CONFIRMED action.
     *        RGB LED -> GREEN for 1s, Dedicated Servo -> CLOSED (0 deg).
     */
    void triggerConfirm();

    /**
     * @brief Hardware response to manual RESET.
     *        RGB LED -> OFF, Dedicated Servo -> CLOSED, State -> IDLE.
     */
    void triggerReset();

    // Query methods
    uint8_t getId() const { return _cellId; }
    CellState getState() const { return _state; }
    bool isAlerting() const { return _state == CELL_STATE_ALERTING; }
    bool isWarning() const { return _state == CELL_STATE_WARNING; }
    bool isConfirmed() const { return _state == CELL_STATE_CONFIRMED; }
    bool isIdle() const { return _state == CELL_STATE_IDLE; }

    RgbLed& getLed() { return _rgb; }
    DebouncedButton& getButton() { return _button; }
    ServoController& getServo() { return _servo; }
};

#endif // CELL_HARDWARE_H
