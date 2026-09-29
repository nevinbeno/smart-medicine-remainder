#ifndef HARDWARE_BRIDGE_H
#define HARDWARE_BRIDGE_H

#include <Arduino.h>

/**
 * @file HardwareBridge.h
 * @brief Integration bridge between Role 4 (4-Cell Hardware & Alert Control)
 *        and Role 2 (Scheduling & Event Engine) / Role 3 (OLED UI).
 *
 * Encapsulates the hardware controller and isolates header namespaces
 * to prevent enum collisions between CellState (hardware) and CellState (occupancy).
 */

// Function pointer for when physical button confirms a cell
typedef void (*BridgeConfirmCallback)(uint8_t cellIndex);

// Initialize all 4 cells (RGB LEDs, debounced buttons, servos) and common buzzer
void initHardwareBridge(BridgeConfirmCallback onConfirm);

// Periodic update loop (call continuously in loop())
void updateHardwareBridge();

// Inbound hardware event triggers
bool triggerHardwareMedicineDue(uint8_t cellIndex);
bool triggerHardwareMissedDose(uint8_t cellIndex);
bool triggerHardwareDoseConfirmed(uint8_t cellIndex);
bool triggerHardwareReset(uint8_t cellIndex);
void triggerHardwareResetAll();

// Queries
uint8_t getHardwareCellCount();
uint8_t getHardwareActiveAlertCount();
const char* getHardwareCellStateString(uint8_t cellIndex);
bool isHardwareCellServoOpen(uint8_t cellIndex);
uint8_t getHardwareCellServoAngle(uint8_t cellIndex);

// Diagnostic status dump
void printHardwareStatus();

#endif // HARDWARE_BRIDGE_H
