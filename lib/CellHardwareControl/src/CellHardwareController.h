#ifndef CELL_HARDWARE_CONTROLLER_H
#define CELL_HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include "HardwareConfig.h"
#include "CellHardwareTypes.h"
#include "CellHardware.h"
#include "BuzzerController.h"

/**
 * @file CellHardwareController.h
 * @brief Master coordinator for the 4-cell medicine reminder system.
 *
 * Implements:
 * - Exactly 4 independent cells, each with dedicated RGB LED, Servo motor, and Pushbutton
 * - Common/shared buzzer alert system with multi-cell arbitration
 * - Simultaneous event handling (confirming one cell does not stop alert for another)
 * - Safe modular architecture
 */
class CellHardwareController {
private:
    CellHardware* _cells[MAX_CELLS];
    uint8_t _cellCount;

    BuzzerController _buzzer;

    DoseConfirmedCallback _doseConfirmedCallback;
    CellStateChangedCallback _stateChangedCallback;

    uint8_t calculateAlertingCount() const;
    void syncBuzzer();

public:
    /**
     * @brief Construct the hardware controller.
     * @param cellCount Number of active cells (defaults to ACTIVE_CELL_COUNT = 4).
     * @param pinConfigs Array of pin configuration structs.
     * @param pinBuzzer GPIO pin for common alert buzzer.
     */
    CellHardwareController(uint8_t cellCount = ACTIVE_CELL_COUNT,
                           const CellPinConfig* pinConfigs = DEFAULT_CELL_PIN_CONFIGS,
                           int8_t pinBuzzer = PIN_COMMON_BUZZER);

    ~CellHardwareController();

    /**
     * @brief Initialize all 4 cell GPIOs, debouncers, RGB LEDs, servos, and the common buzzer.
     */
    void begin();

    /**
     * @brief Non-blocking master update loop. Call continuously in main loop().
     */
    void update();

    // ========================================================================
    // INBOUND EVENT INTERFACES
    // ========================================================================

    /**
     * @brief Inbound Event: MEDICINE_DUE(cellId).
     *        Triggers RED LED and OPENS SERVO for ONLY that cell, activates common buzzer.
     * @param cellId 0-indexed cell identifier (0 to 3).
     * @return True if event was accepted and processed.
     */
    bool triggerMedicineDue(uint8_t cellId);

    /**
     * @brief Inbound Event: MISSED_DOSE(cellId).
     *        Triggers YELLOW LED and CLOSES SERVO for ONLY that cell.
     * @param cellId 0-indexed cell identifier (0 to 3).
     * @return True if event was accepted and processed.
     */
    bool triggerMissedDose(uint8_t cellId);

    /**
     * @brief Inbound Event: WARNING(cellId). Alias for triggerMissedDose.
     */
    bool triggerWarning(uint8_t cellId) { return triggerMissedDose(cellId); }

    /**
     * @brief Programmatically trigger dose confirmation for a cell.
     *        Triggers GREEN LED for 1s, closes servo for that cell, fires DOSE_CONFIRMED callback.
     * @param cellId 0-indexed cell identifier (0 to 3).
     * @return True if event was accepted and processed.
     */
    bool triggerDoseConfirmed(uint8_t cellId);

    /**
     * @brief Inbound Event: RESET_CELL(cellId).
     *        Forces cell back to IDLE state (LED off, servo closed).
     * @param cellId 0-indexed cell identifier (0 to 3).
     * @return True if event was accepted and processed.
     */
    bool triggerReset(uint8_t cellId);

    /**
     * @brief Force reset all 4 cells to IDLE and silence buzzer.
     */
    void triggerResetAll();

    // ========================================================================
    // OUTBOUND EVENT REGISTRATION
    // ========================================================================

    void setConfirmationCallback(DoseConfirmedCallback callback);
    void setStateChangedCallback(CellStateChangedCallback callback);

    // ========================================================================
    // STATUS & QUERY METHODS
    // ========================================================================

    uint8_t getCellCount() const { return _cellCount; }
    uint8_t getActiveAlertCount() const { return calculateAlertingCount(); }
    CellState getCellState(uint8_t cellId) const;
    bool isCellAlerting(uint8_t cellId) const;
    bool isCellWarning(uint8_t cellId) const;
    bool isCellConfirmed(uint8_t cellId) const;
    bool isCellIdle(uint8_t cellId) const;

    BuzzerController& getBuzzer() { return _buzzer; }
    CellHardware* getCell(uint8_t cellId);
};

#endif // CELL_HARDWARE_CONTROLLER_H
