#ifndef TIME_PROVIDER_HPP
#define TIME_PROVIDER_HPP

#include "time_engine_types.hpp"

/**
 * @brief Abstract interface for querying current system time.
 * Decouples the engine from specific hardware RTC chips, enabling
 * seamless simulation, fast-forwarding, and automated unit testing.
 */
class ITimeProvider
{
public:
    virtual ~ITimeProvider() = default;

    /**
     * @brief Get current 24-hour time of day.
     */
    virtual TimeOfDay getCurrentTime() = 0;

    /**
     * @brief Get current epoch timestamp in seconds.
     */
    virtual uint32_t getEpochSeconds() = 0;

    /**
     * @brief Check whether the time provider is initialized and running.
     */
    virtual bool isAvailable() = 0;
};

/**
 * @brief Deterministic simulated time provider.
 * Allows setting explicit time and advancing seconds/minutes for testing edge cases
 * such as simultaneous doses, missed alarms, and midnight rollovers.
 */
class SimulatedTimeProvider : public ITimeProvider
{
public:
    SimulatedTimeProvider(uint8_t initialHour = 0, uint8_t initialMinute = 0, uint8_t initialSecond = 0);

    TimeOfDay getCurrentTime() override;
    uint32_t getEpochSeconds() override;
    bool isAvailable() override;

    void setTime(uint8_t hour, uint8_t minute, uint8_t second = 0);
    void advanceSeconds(uint32_t seconds);
    void advanceMinutes(uint32_t minutes);
    void advanceHours(uint32_t hours);

    void setEpochSeconds(uint32_t epoch);

private:
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t epochSeconds;
    bool available;

    void normalizeTime();
};

#if defined(ARDUINO) || defined(ESP32)
// Forward declare RTClib classes to avoid hard header dependency if not compiled with RTC
class RTC_DS1307;
class RTC_DS3231;

/**
 * @brief Hardware Real-Time Clock (DS1307/DS3231) provider using RTClib.
 */
class RTCTimeProvider : public ITimeProvider
{
public:
    RTCTimeProvider();

    TimeOfDay getCurrentTime() override;
    uint32_t getEpochSeconds() override;
    bool isAvailable() override;

    void setAvailable(bool state);

private:
    bool available;
};
#endif

#endif // TIME_PROVIDER_HPP
