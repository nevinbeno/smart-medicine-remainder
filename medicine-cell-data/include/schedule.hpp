#ifndef SCHEDULE_HPP
#define SCHEDULE_HPP

#include <cstdint>

struct ScheduleEntry
{
    uint8_t hour;
    uint8_t minute;
    uint16_t doseQuantity;
};

#endif