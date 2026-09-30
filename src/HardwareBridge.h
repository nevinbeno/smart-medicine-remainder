#ifndef HARDWARE_BRIDGE_H
#define HARDWARE_BRIDGE_H

#include <Arduino.h>

/**
 * @file HardwareBridge.h
 * @brief Integration bridge between Role 4 (Cell Hardware & Alert Control)
 *        and Role 2 (Scheduling & Event Engine) / Role 3 (OLED UI).
 *
 * Encapsulates the hardware controller and isolates header namespaces
 * to prevent enum collisions between CellState (hardware) and CellState (occupancy).
 */

// Function pointer for when physical button confirms a cell
typedef void (*BridgeConfirmCallback)(uint8_t cellIndex);

// Initialize cells (RGB LEDs, debounced buttons, servos) and common buzzer.
// Takes activeCellCount as input (defaults to 4).
void initHardwareBridge(BridgeConfirmCallback onConfirm, uint8_t activeCellCount = 4);

// Dynamically set the number of active cells at runtime (takes cell count as input)
bool setHardwareCellCount(uint8_t cellCount);

// Scalable Matrix Grid configuration (e.g. 2x2, 4x4, 8x8)
bool setHardwareGrid(uint8_t rows, uint8_t cols);
uint8_t getHardwareGridRows();
uint8_t getHardwareGridCols();
int8_t getHardwareCellIndex(uint8_t row, uint8_t col);
bool getHardwareCellGridCoords(uint8_t cellIndex, uint8_t& outRow, uint8_t& outCol);

// Periodic update loop (call continuously in loop())
void updateHardwareBridge();

// Inbound hardware event triggers (linear 0-indexed)
bool triggerHardwareMedicineDue(uint8_t cellIndex);
bool triggerHardwareMissedDose(uint8_t cellIndex);
bool triggerHardwareDoseConfirmed(uint8_t cellIndex);
bool triggerHardwareReset(uint8_t cellIndex);
void triggerHardwareResetAll();

// Inbound hardware event triggers (matrix 1-indexed: row, col)
bool triggerHardwareMedicineDueGrid(uint8_t row, uint8_t col);
bool triggerHardwareMissedDoseGrid(uint8_t row, uint8_t col);
bool triggerHardwareDoseConfirmedGrid(uint8_t row, uint8_t col);
bool triggerHardwareResetGrid(uint8_t row, uint8_t col);

// Queries
uint8_t getHardwareCellCount();
uint8_t getHardwareActiveAlertCount();
const char* getHardwareCellStateString(uint8_t cellIndex);
bool isHardwareCellServoOpen(uint8_t cellIndex);
uint8_t getHardwareCellServoAngle(uint8_t cellIndex);
bool isHardwareCellVirtual(uint8_t cellIndex);

// Simulate button press for virtual-expander cells (5+) via serial PRESS.
// Flows through update() -> scheduler callback, same as physical Btn 1-4.
bool pressHardwareButton(uint8_t cellIndex);
bool pressHardwareButtonGrid(uint8_t row, uint8_t col);

// Diagnostic status dump
void printHardwareStatus();

#endif // HARDWARE_BRIDGE_H
