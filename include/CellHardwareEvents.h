#ifndef CELL_HARDWARE_EVENTS_H
#define CELL_HARDWARE_EVENTS_H

/**
 * @file CellHardwareEvents.h
 * @brief Public integration interface for Role 4: Cell Hardware & Alert Control.
 *
 * This header defines the exact boundary and contract between Role 4 (Cell Hardware)
 * and all other project roles:
 *
 * - Role 2 (Scheduling & Time/Event Engine) -> Calls inbound events:
 *     - MEDICINE_DUE(cell_id)  -> hardwareController.triggerMedicineDue(cell_id)
 *     - MISSED_DOSE(cell_id)   -> hardwareController.triggerMissedDose(cell_id)
 *     - RESET_CELL(cell_id)    -> hardwareController.triggerReset(cell_id)
 *
 * - Role 5 (Dose/Stock Management) -> Registers for outbound events:
 *     - DOSE_CONFIRMED(cell_id)-> hardwareController.setConfirmationCallback(myCallback)
 *     (Role 5 decrements stock and records history upon receiving this event)
 *
 * - Role 3 (OLED UI & Notification Engine) -> Listens to state changes:
 *     - hardwareController.setStateChangedCallback(myUiCallback)
 *     - hardwareController.getCellState(cell_id)
 */

#include "../lib/CellHardwareControl/src/HardwareConfig.h"
#include "../lib/CellHardwareControl/src/CellHardwareTypes.h"
#include "../lib/CellHardwareControl/src/CellHardwareController.h"

// Global controller instance declaration (defined in src/main.cpp)
extern CellHardwareController cellHardware;

// ============================================================================
// CONVENIENCE INBOUND MACROS / WRAPPERS FOR TEAM MATES
// ============================================================================

/**
 * @brief Teammate API: Signal that a scheduled dose is due.
 * @param cell_id 0-indexed compartment identifier.
 */
inline bool EVENT_MEDICINE_DUE(uint8_t cell_id) {
    return cellHardware.triggerMedicineDue(cell_id);
}

/**
 * @brief Teammate API: Signal that a dose became missed.
 * @param cell_id 0-indexed compartment identifier.
 */
inline bool EVENT_MISSED_DOSE(uint8_t cell_id) {
    return cellHardware.triggerMissedDose(cell_id);
}

/**
 * @brief Teammate API: Signal manual reset/clear of a cell.
 * @param cell_id 0-indexed compartment identifier.
 */
inline bool EVENT_RESET_CELL(uint8_t cell_id) {
    return cellHardware.triggerReset(cell_id);
}

#endif // CELL_HARDWARE_EVENTS_H
