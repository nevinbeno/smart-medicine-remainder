#ifndef TIME_ENGINE_TYPES_HPP
#define TIME_ENGINE_TYPES_HPP

#include <cstdint>
#include <cstring>

#ifndef MAX_ENGINE_CELLS
#define MAX_ENGINE_CELLS 16
#endif

#ifndef MAX_SCHEDULES_PER_CELL
#define MAX_SCHEDULES_PER_CELL 8
#endif

#ifndef MAX_ACTIVE_EVENTS
#define MAX_ACTIVE_EVENTS 16
#endif

#ifndef MAX_MISSED_EVENTS
#define MAX_MISSED_EVENTS 16
#endif

#ifndef MAX_ENGINE_STRING_LEN
#define MAX_ENGINE_STRING_LEN 32
#endif

#ifndef MAX_ENGINE_ID_LEN
#define MAX_ENGINE_ID_LEN 16
#endif

#ifndef MAX_EVENT_LISTENERS
#define MAX_EVENT_LISTENERS 8
#endif

/**
 * @brief Represents the state of a physical compartment/cell.
 * Matches CellState defined in medicine-cell-data.
 */
#if defined(DATA_TYPES_HPP)
// Already defined in data_types.hpp
#elif __has_include("data_types.hpp")
#include "data_types.hpp"
#elif __has_include("../../medicine-cell-data/include/data_types.hpp")
#include "../../medicine-cell-data/include/data_types.hpp"
#else
enum class CellState
{
    EMPTY = 0,
    OCCUPIED = 1,
    DISABLED_STATE = 2
};
#endif

/**
 * @brief Types of events emitted by the Scheduling & Event Engine.
 */
enum class EventType
{
    NONE = 0,
    DOSE_DUE,            ///< Scheduled time reached; hardware alert active (buzzer + cell LED)
    DOSE_MISSED,         ///< Grace period expired without confirmation; notification displayed on OLED
    DOSE_CONFIRMED,      ///< User pressed confirm button for the cell during due window
    DOSE_DISMISSED,      ///< User dismissed a missed dose notification
    DAILY_RESET,         ///< Midnight rollover occurred; daily triggers refreshed
    LOW_STOCK_WARNING    ///< Stock level dropped below threshold
};

/**
 * @brief Lifecycle status of a specific dose event.
 */
enum class EventState
{
    IDLE = 0,            ///< Scheduled but not yet due
    DUE,                 ///< Actively alerting; awaiting patient confirmation
    MISSED,              ///< Due window elapsed; reminder pending on OLED
    CONFIRMED,           ///< Successfully confirmed by patient
    DISMISSED            ///< Missed reminder dismissed by patient
};

/**
 * @brief Simple 24-hour time representation (HH:MM:SS).
 */
struct TimeOfDay
{
    uint8_t hour;        ///< 0 - 23
    uint8_t minute;      ///< 0 - 59
    uint8_t second;      ///< 0 - 59

    TimeOfDay() : hour(0), minute(0), second(0) {}
    TimeOfDay(uint8_t h, uint8_t m, uint8_t s = 0) : hour(h), minute(m), second(s) {}

    uint32_t toSecondsOfDay() const
    {
        return static_cast<uint32_t>(hour) * 3600UL +
               static_cast<uint32_t>(minute) * 60UL +
               static_cast<uint32_t>(second);
    }

    uint32_t toMinutesOfDay() const
    {
        return static_cast<uint32_t>(hour) * 60UL + static_cast<uint32_t>(minute);
    }

    bool operator==(const TimeOfDay& other) const
    {
        return hour == other.hour && minute == other.minute && second == other.second;
    }

    bool operator!=(const TimeOfDay& other) const
    {
        return !(*this == other);
    }

    bool operator<(const TimeOfDay& other) const
    {
        return toSecondsOfDay() < other.toSecondsOfDay();
    }

    bool operator<=(const TimeOfDay& other) const
    {
        return toSecondsOfDay() <= other.toSecondsOfDay();
    }

    bool isValid() const
    {
        return hour < 24 && minute < 60 && second < 60;
    }
};

/**
 * @brief Represents an individual dose schedule for a medicine.
 */
struct DoseSchedule
{
    uint8_t hour;             ///< Scheduled hour (0-23)
    uint8_t minute;           ///< Scheduled minute (0-59)
    uint16_t doseQuantity;    ///< Number of units/tablets per dose
    bool triggeredToday;      ///< Whether this schedule has fired today
    bool enabled;             ///< Whether this schedule slot is active

    DoseSchedule()
        : hour(0), minute(0), doseQuantity(1), triggeredToday(false), enabled(false) {}

    DoseSchedule(uint8_t h, uint8_t m, uint16_t qty = 1)
        : hour(h), minute(m), doseQuantity(qty), triggeredToday(false), enabled(true) {}
};

/**
 * @brief Configuration data for a single compartment/cell inside the engine.
 */
struct EngineCell
{
    char cellId[MAX_ENGINE_ID_LEN];
    uint8_t cellIndex;
    CellState state;
    uint16_t currentStock;
    uint16_t lowStockThreshold;

    char medicineId[MAX_ENGINE_ID_LEN];
    char medicineName[MAX_ENGINE_STRING_LEN];
    bool hasMedicine;

    DoseSchedule schedules[MAX_SCHEDULES_PER_CELL];
    uint8_t scheduleCount;

    EngineCell()
        : cellIndex(0), state(CellState::EMPTY), currentStock(0), lowStockThreshold(2),
          hasMedicine(false), scheduleCount(0)
    {
        cellId[0] = '\0';
        medicineId[0] = '\0';
        medicineName[0] = '\0';
    }
};

/**
 * @brief Represents an active or historical dose event.
 */
struct DoseEvent
{
    uint32_t eventId;
    EventType type;
    EventState state;

    char cellId[MAX_ENGINE_ID_LEN];
    uint8_t cellIndex;

    char medicineId[MAX_ENGINE_ID_LEN];
    char medicineName[MAX_ENGINE_STRING_LEN];
    uint16_t doseQuantity;

    TimeOfDay scheduledTime;
    TimeOfDay triggeredTime;
    TimeOfDay resolvedTime;

    uint32_t triggeredEpoch;   ///< Epoch or second counter when event triggered

    DoseEvent()
        : eventId(0), type(EventType::NONE), state(EventState::IDLE),
          cellIndex(0), doseQuantity(1), triggeredEpoch(0)
    {
        cellId[0] = '\0';
        medicineId[0] = '\0';
        medicineName[0] = '\0';
    }
};

/**
 * @brief Callback function signature for event listeners.
 */
typedef void (*EventCallbackFn)(const DoseEvent& event, void* userData);

#endif // TIME_ENGINE_TYPES_HPP
