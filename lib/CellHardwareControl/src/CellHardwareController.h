#ifndef CELL_HARDWARE_CONTROLLER_H
#define CELL_HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include "CellHardware.h"

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
    CellHardware* _cells[HW_MAX_CELLS];
    uint8_t _cellCount;
    uint8_t _gridRows;
    uint8_t _gridCols;

    BuzzerController _buzzer;

    DoseConfirmedCallback _doseConfirmedCallback;
    CellStateChangedCallback _stateChangedCallback;

    uint8_t calculateAlertingCount() const;
    void syncBuzzer();

public:
    /**
     * @brief Construct the hardware controller.
     * @param cellCount Number of active cells (defaults to DEFAULT_CELL_COUNT = 4).
     * @param pinConfigs Array of pin configuration structs.
     * @param pinBuzzer GPIO pin for common alert buzzer.
     */
    CellHardwareController(uint8_t cellCount = DEFAULT_CELL_COUNT,
                           const CellPinConfig* pinConfigs = DEFAULT_CELL_PIN_CONFIGS,
                           int8_t pinBuzzer = PIN_COMMON_BUZZER);

    ~CellHardwareController();

    /**
     * @brief Initialize all cell GPIOs, debouncers, RGB LEDs, servos, and the common buzzer.
     */
    void begin();

    /**
     * @brief Non-blocking master update loop. Call continuously in main loop().
     */
    void update();

    /**
     * @brief Dynamically set the number of active cells at runtime.
     * @param count Desired active cell count (1 to HW_MAX_CELLS).
     * @return True if valid and applied, false otherwise.
     */
    bool setCellCount(uint8_t count);

    /**
     * @brief Configure scalable matrix grid dimensions (e.g. 2x2, 4x4, 8x8).
     * @param rows Number of rows (1 to HW_MAX_GRID_ROWS).
     * @param cols Number of columns (1 to HW_MAX_GRID_COLS).
     * @return True if valid and applied, false otherwise.
     */
    bool setGrid(uint8_t rows, uint8_t cols);

    uint8_t getGridRows() const { return _gridRows; }
    uint8_t getGridCols() const { return _gridCols; }

    /**
     * @brief Convert 1-indexed (row, col) matrix coordinates to 0-indexed linear cell index.
     * @return 0-indexed cell index, or -1 if coordinates are out of bounds.
     */
    int8_t gridCoordsToIndex(uint8_t row, uint8_t col) const;

    /**
     * @brief Convert 0-indexed linear cell index to 1-indexed (row, col) matrix coordinates.
     */
    bool indexToGridCoords(uint8_t cellIndex, uint8_t& outRow, uint8_t& outCol) const;

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
     * @brief Force reset all active cells to IDLE and silence buzzer.
     */
    void triggerResetAll();

    // ========================================================================
    // MATRIX GRID COORDINATE TRIGGERS (1-INDEXED ROW, COL)
    // ========================================================================
    bool triggerMedicineDueGrid(uint8_t row, uint8_t col);
    bool triggerMissedDoseGrid(uint8_t row, uint8_t col);
    bool triggerWarningGrid(uint8_t row, uint8_t col) { return triggerMissedDoseGrid(row, col); }
    bool triggerDoseConfirmedGrid(uint8_t row, uint8_t col);
    bool triggerResetGrid(uint8_t row, uint8_t col);

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
    CellState getCellStateGrid(uint8_t row, uint8_t col) const;
    bool isCellAlerting(uint8_t cellId) const;
    bool isCellWarning(uint8_t cellId) const;
    bool isCellConfirmed(uint8_t cellId) const;
    bool isCellIdle(uint8_t cellId) const;
    bool isCellVirtual(uint8_t cellId) const;

    // Simulate a button press for virtual-expander cells (5+). Works for any
    // active cell; flows through update() -> confirmation callback -> scheduler.
    bool injectVirtualButtonPress(uint8_t cellId);
    bool injectVirtualButtonPressGrid(uint8_t row, uint8_t col);

    BuzzerController& getBuzzer() { return _buzzer; }
    CellHardware* getCell(uint8_t cellId);
};

#endif // CELL_HARDWARE_CONTROLLER_H
