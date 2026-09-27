#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <cstdint>

#include "cell.hpp"
#include "medicine.hpp"
#include "schedule.hpp"

constexpr uint8_t MAX_CELLS = 16;

constexpr uint8_t MAX_MEDICINE_ID_LENGTH = 16;
constexpr uint8_t MAX_MEDICINE_NAME_LENGTH = 32;
constexpr uint8_t MAX_CELL_ID_LENGTH = 16;

struct RuntimeMedicine
{
    char medicineId[MAX_MEDICINE_ID_LENGTH];
    char name[MAX_MEDICINE_NAME_LENGTH];

    uint16_t lowStockThreshold;

    ScheduleEntry schedules[MAX_SCHEDULES];
    uint8_t scheduleCount;
};

struct RuntimeCell
{
    char cellId[MAX_CELL_ID_LENGTH];

    CellState state;
    uint16_t currentStock;

    RuntimeMedicine medicine;

    bool hasMedicine;
};

struct RuntimeConfiguration
{
    uint8_t cellCount;
    RuntimeCell cells[MAX_CELLS];
};

#endif