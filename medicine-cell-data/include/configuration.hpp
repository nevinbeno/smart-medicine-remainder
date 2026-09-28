#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <cstdint>
#include "data_types.hpp"

/**
 * @file configuration.hpp
 * @brief Runtime configuration structures for the Smart Medicine Reminder system.
 *        Defines RuntimeMedicine, RuntimeCell, and RuntimeConfiguration used by the
 *        JSON configuration loader and the scheduling engine adapter.
 */

constexpr uint8_t MAX_CELLS             = 16;
constexpr uint8_t MAX_MEDICINE_ID_LENGTH = 16;
constexpr uint8_t MAX_MEDICINE_NAME_LENGTH = 32;
constexpr uint8_t MAX_CELL_ID_LENGTH    = 16;

struct RuntimeMedicine
{
    char     medicineId[MAX_MEDICINE_ID_LENGTH];
    char     name[MAX_MEDICINE_NAME_LENGTH];
    uint16_t lowStockThreshold;
    ScheduleEntry schedules[MAX_SCHEDULES];
    uint8_t  scheduleCount;
};

struct RuntimeCell
{
    char          cellId[MAX_CELL_ID_LENGTH];
    CellState     state;
    uint16_t      currentStock;
    RuntimeMedicine medicine;
    bool          hasMedicine;
};

struct RuntimeConfiguration
{
    uint8_t    cellCount;
    RuntimeCell cells[MAX_CELLS];
};

#endif // CONFIGURATION_HPP