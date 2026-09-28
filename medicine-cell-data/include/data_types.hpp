#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include <cstdint>

/**
 * @file data_types.hpp
 * @brief Consolidated runtime types for the medicine-cell-data module.
 *        Defines the CellState occupancy enum, ScheduleEntry, Medicine, and Cell types
 *        used by the configuration loader and scheduling engine.
 */

// ---------------------------------------------------------------------------
// Schedule entry: a single timed dose slot for a medicine
// ---------------------------------------------------------------------------

struct ScheduleEntry
{
    uint8_t  hour;
    uint8_t  minute;
    uint16_t doseQuantity;
};

// ---------------------------------------------------------------------------
// Cell occupancy state (distinct from hardware CellState in CellHardwareTypes.h)
// ---------------------------------------------------------------------------

enum class CellState
{
    EMPTY          = 0,
    OCCUPIED       = 1,
    DISABLED_STATE = 2
};

// ---------------------------------------------------------------------------
// Medicine runtime descriptor
// ---------------------------------------------------------------------------

constexpr uint8_t MAX_SCHEDULES = 8;

struct Medicine
{
    const char* medicineId;
    const char* name;
    uint16_t    lowStockThreshold;
    ScheduleEntry schedules[MAX_SCHEDULES];
    uint8_t     scheduleCount;
};

// ---------------------------------------------------------------------------
// Cell runtime descriptor
// ---------------------------------------------------------------------------

class Cell
{
public:
    explicit Cell(uint8_t cellId);

    bool assignMedicine(const char* medicineId);
    bool removeMedicine();
    bool setStock(uint16_t stock);
    bool enable();
    bool disable();

    uint8_t     getCellId()     const;
    const char* getMedicineId() const;
    uint16_t    getStock()      const;
    CellState   getState()      const;
    bool        isEnabled()     const;

private:
    uint8_t     cellId;
    const char* assignedMedicineId;
    uint16_t    currentStock;
    CellState   state;
};

#endif // DATA_TYPES_HPP
