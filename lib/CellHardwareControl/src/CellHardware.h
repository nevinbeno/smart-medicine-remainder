#ifndef CELL_HARDWARE_H
#define CELL_HARDWARE_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "CellHardwareTypes.h"
#include "CellActuators.h"

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
    bool _virtualPressPending;

    void applyStateHardware(CellState newState);
    void logVirtualTransition(CellState newState) const;

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

    // Virtual-expander cells (id >= HW_DIRECT_PIN_CELLS) have no direct GPIO
    // (pins == -1). They are fully functional in logic + serial/OLED/buzzer,
    // with LED/servo state tracked and logged. Use injectButtonPress() or
    // serial PRESS <id> to simulate their button.
    bool isVirtual() const;

    RgbLed& getLed() { return _rgb; }
    DebouncedButton& getButton() { return _button; }
    ServoController& getServo() { return _servo; }

    // Simulate a physical button press for virtual (or physical) cells.
    // The next update() call returns true once, flowing through the normal
    // debounce -> confirm -> scheduler callback path.
    void injectButtonPress() { _virtualPressPending = true; }
};

#endif // CELL_HARDWARE_H
