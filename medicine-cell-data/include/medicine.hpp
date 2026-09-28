#ifndef MEDICINE_HPP
#define MEDICINE_HPP

#include <cstdint>
#include "schedule.hpp"

constexpr uint8_t MAX_SCHEDULES = 8;

struct Medicine
{
    const char* medicineId;
    const char* name;

    uint16_t lowStockThreshold;

    ScheduleEntry schedules[MAX_SCHEDULES];
    uint8_t scheduleCount;
};

#endif