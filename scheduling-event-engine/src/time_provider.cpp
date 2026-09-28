#include "../include/time_provider.hpp"

SimulatedTimeProvider::SimulatedTimeProvider(uint8_t initialHour, uint8_t initialMinute, uint8_t initialSecond)
    : hour(initialHour), minute(initialMinute), second(initialSecond),
      epochSeconds(0), available(true)
{
    normalizeTime();
}

TimeOfDay SimulatedTimeProvider::getCurrentTime()
{
    return TimeOfDay(hour, minute, second);
}

uint32_t SimulatedTimeProvider::getEpochSeconds()
{
    if (epochSeconds > 0)
    {
        return epochSeconds;
    }
    return static_cast<uint32_t>(hour) * 3600UL +
           static_cast<uint32_t>(minute) * 60UL +
           static_cast<uint32_t>(second);
}

bool SimulatedTimeProvider::isAvailable()
{
    return available;
}

void SimulatedTimeProvider::setTime(uint8_t h, uint8_t m, uint8_t s)
{
    hour = h;
    minute = m;
    second = s;
    normalizeTime();
}

void SimulatedTimeProvider::advanceSeconds(uint32_t secondsToAdd)
{
    uint32_t totalSec = static_cast<uint32_t>(second) + secondsToAdd;
    second = static_cast<uint8_t>(totalSec % 60);

    uint32_t totalMin = static_cast<uint32_t>(minute) + (totalSec / 60);
    minute = static_cast<uint8_t>(totalMin % 60);

    uint32_t totalHr = static_cast<uint32_t>(hour) + (totalMin / 60);
    hour = static_cast<uint8_t>(totalHr % 24);

    if (epochSeconds > 0)
    {
        epochSeconds += secondsToAdd;
    }
}

void SimulatedTimeProvider::advanceMinutes(uint32_t minutesToAdd)
{
    advanceSeconds(minutesToAdd * 60UL);
}

void SimulatedTimeProvider::advanceHours(uint32_t hoursToAdd)
{
    advanceSeconds(hoursToAdd * 3600UL);
}

void SimulatedTimeProvider::setEpochSeconds(uint32_t epoch)
{
    epochSeconds = epoch;
}

void SimulatedTimeProvider::normalizeTime()
{
    if (second >= 60)
    {
        minute += second / 60;
        second %= 60;
    }
    if (minute >= 60)
    {
        hour += minute / 60;
        minute %= 60;
    }
    if (hour >= 24)
    {
        hour %= 24;
    }
}

#if defined(ARDUINO) || defined(ESP32)
#include <RTClib.h>

extern RTC_DS1307 rtc;

RTCTimeProvider::RTCTimeProvider()
    : available(true)
{
}

TimeOfDay RTCTimeProvider::getCurrentTime()
{
    DateTime now = rtc.now();
    return TimeOfDay(now.hour(), now.minute(), now.second());
}

uint32_t RTCTimeProvider::getEpochSeconds()
{
    DateTime now = rtc.now();
    return now.unixtime();
}

bool RTCTimeProvider::isAvailable()
{
    return available;
}

void RTCTimeProvider::setAvailable(bool state)
{
    available = state;
}
#endif
