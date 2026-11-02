#include "../include/schedule_event_engine.hpp"

ScheduleEventEngine::ScheduleEventEngine()
    : timeProvider(nullptr), gracePeriodMinutes(10), lowStockWarningEnabled(true),
      cellCount(0), dueEventCount(0), missedEventCount(0), callbackCount(0),
      nextEventId(0), firstRun(true)
{
    for (uint8_t i = 0; i < MAX_ENGINE_CELLS; i++) { lastWarnedStock[i] = 0xFFFFU; }
}

ScheduleEventEngine::ScheduleEventEngine(ITimeProvider* provider)
    : timeProvider(provider), gracePeriodMinutes(10), lowStockWarningEnabled(true),
      cellCount(0), dueEventCount(0), missedEventCount(0), callbackCount(0),
      nextEventId(0), firstRun(true)
{
    for (uint8_t i = 0; i < MAX_ENGINE_CELLS; i++) { lastWarnedStock[i] = 0xFFFFU; }
}

void ScheduleEventEngine::setTimeProvider(ITimeProvider* provider)
{
    timeProvider = provider;
}

ITimeProvider* ScheduleEventEngine::getTimeProvider() const
{
    return timeProvider;
}

void ScheduleEventEngine::setGracePeriodMinutes(uint16_t minutes)
{
    gracePeriodMinutes = (minutes > 0) ? minutes : 1;
}

uint16_t ScheduleEventEngine::getGracePeriodMinutes() const
{
    return gracePeriodMinutes;
}

void ScheduleEventEngine::setLowStockWarningEnabled(bool enabled)
{
    lowStockWarningEnabled = enabled;
}

bool ScheduleEventEngine::isLowStockWarningEnabled() const
{
    return lowStockWarningEnabled;
}

bool ScheduleEventEngine::addCell(const EngineCell& cell)
{
    if (cellCount >= MAX_ENGINE_CELLS)
    {
        return false;
    }
    cells[cellCount] = cell;
    cells[cellCount].cellIndex = cellCount;
    cellCount++;
    return true;
}

bool ScheduleEventEngine::updateCell(uint8_t cellIndex, const EngineCell& cell)
{
    if (cellIndex >= cellCount)
    {
        return false;
    }
    cells[cellIndex] = cell;
    cells[cellIndex].cellIndex = cellIndex;
    return true;
}

void ScheduleEventEngine::clearCells()
{
    cellCount = 0;
    dueEventCount = 0;
    missedEventCount = 0;
}

uint8_t ScheduleEventEngine::getCellCount() const
{
    return cellCount;
}

const EngineCell* ScheduleEventEngine::getCell(uint8_t cellIndex) const
{
    if (cellIndex >= cellCount)
    {
        return nullptr;
    }
    return &cells[cellIndex];
}

EngineCell* ScheduleEventEngine::getCellMutable(uint8_t cellIndex)
{
    if (cellIndex >= cellCount)
    {
        return nullptr;
    }
    return &cells[cellIndex];
}

int16_t ScheduleEventEngine::findCellIndexById(const char* cellId) const
{
    if (!cellId)
    {
        return -1;
    }
    for (uint8_t i = 0; i < cellCount; i++)
    {
        if (strncmp(cells[i].cellId, cellId, MAX_ENGINE_ID_LEN) == 0)
        {
            return static_cast<int16_t>(i);
        }
    }
    return -1;
}

bool ScheduleEventEngine::updateCellStock(uint8_t cellIndex, uint16_t newStock)
{
    if (cellIndex >= cellCount)
    {
        return false;
    }
    cells[cellIndex].currentStock = newStock;
    if (newStock > cells[cellIndex].lowStockThreshold)
    {
        lastWarnedStock[cellIndex] = 0xFFFFU;
    }
    return true;
}

bool ScheduleEventEngine::triggerDueEvent(uint8_t cellIndex)
{
    if (cellIndex >= cellCount)
    {
        return false;
    }

    if (cells[cellIndex].state != CellState::OCCUPIED || !cells[cellIndex].hasMedicine)
    {
        return false;
    }

    if (cells[cellIndex].currentStock == 0)
    {
        // Out of stock: cannot trigger due dose
        return false;
    }

    if (isCellDue(cellIndex))
    {
        return true; // Already due
    }

    if (dueEventCount >= MAX_ACTIVE_EVENTS)
    {
        return false;
    }

    TimeOfDay currentTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

    DoseEvent event;
    event.eventId = ++nextEventId;
    event.type = EventType::DOSE_DUE;
    event.state = EventState::DUE;

    strncpy(event.cellId, cells[cellIndex].cellId, MAX_ENGINE_ID_LEN - 1);
    event.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';
    event.cellIndex = cellIndex;

    strncpy(event.medicineId, cells[cellIndex].medicineId, MAX_ENGINE_ID_LEN - 1);
    event.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';

    strncpy(event.medicineName, cells[cellIndex].medicineName, MAX_ENGINE_STRING_LEN - 1);
    event.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';

    uint16_t qty = 1;
    if (cells[cellIndex].scheduleCount > 0)
    {
        qty = cells[cellIndex].schedules[0].doseQuantity;
        for (uint8_t s = 0; s < cells[cellIndex].scheduleCount; s++)
        {
            if (cells[cellIndex].schedules[s].hour == currentTime.hour &&
                cells[cellIndex].schedules[s].minute == currentTime.minute)
            {
                qty = cells[cellIndex].schedules[s].doseQuantity;
                break;
            }
        }
    }
    event.doseQuantity = (qty > 0) ? qty : 1;
    event.scheduledTime = currentTime;
    event.triggeredTime = currentTime;
    event.triggeredEpoch = (timeProvider) ? timeProvider->getEpochSeconds() : 0;

    activeDueEvents[dueEventCount++] = event;
    notifyListeners(event);
    return true;
}

bool ScheduleEventEngine::triggerMissedEvent(uint8_t cellIndex)
{
    if (cellIndex >= cellCount)
    {
        return false;
    }

    if (cells[cellIndex].state != CellState::OCCUPIED || !cells[cellIndex].hasMedicine)
    {
        return false;
    }

    if (isCellMissed(cellIndex))
    {
        return true;
    }

    for (uint8_t i = 0; i < dueEventCount; i++)
    {
        if (activeDueEvents[i].cellIndex == cellIndex)
        {
            removeDueEventByIndex(i);
            break;
        }
    }

    if (missedEventCount >= MAX_MISSED_EVENTS)
    {
        return false;
    }

    TimeOfDay currentTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

    DoseEvent event;
    event.eventId = ++nextEventId;
    event.type = EventType::DOSE_MISSED;
    event.state = EventState::MISSED;

    strncpy(event.cellId, cells[cellIndex].cellId, MAX_ENGINE_ID_LEN - 1);
    event.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';
    event.cellIndex = cellIndex;

    strncpy(event.medicineId, cells[cellIndex].medicineId, MAX_ENGINE_ID_LEN - 1);
    event.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';

    strncpy(event.medicineName, cells[cellIndex].medicineName, MAX_ENGINE_STRING_LEN - 1);
    event.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';

    event.doseQuantity = (cells[cellIndex].scheduleCount > 0) ? cells[cellIndex].schedules[0].doseQuantity : 1;
    event.scheduledTime = currentTime;
    event.triggeredTime = currentTime;
    event.triggeredEpoch = (timeProvider) ? timeProvider->getEpochSeconds() : 0;

    activeMissedEvents[missedEventCount++] = event;
    notifyListeners(event);
    return true;
}


bool ScheduleEventEngine::registerEventCallback(EventCallbackFn callback, void* userData)
{
    if (!callback || callbackCount >= MAX_EVENT_LISTENERS)
    {
        return false;
    }
    for (uint8_t i = 0; i < callbackCount; i++)
    {
        if (callbacks[i].fn == callback && callbacks[i].userData == userData)
        {
            return true; // Already registered
        }
    }
    callbacks[callbackCount].fn = callback;
    callbacks[callbackCount].userData = userData;
    callbackCount++;
    return true;
}

bool ScheduleEventEngine::unregisterEventCallback(EventCallbackFn callback)
{
    for (uint8_t i = 0; i < callbackCount; i++)
    {
        if (callbacks[i].fn == callback)
        {
            for (uint8_t j = i; j < callbackCount - 1; j++)
            {
                callbacks[j] = callbacks[j + 1];
            }
            callbackCount--;
            return true;
        }
    }
    return false;
}

void ScheduleEventEngine::notifyListeners(const DoseEvent& event)
{
    for (uint8_t i = 0; i < callbackCount; i++)
    {
        if (callbacks[i].fn)
        {
            callbacks[i].fn(event, callbacks[i].userData);
        }
    }
}

uint32_t ScheduleEventEngine::calculateElapsedMinutes(const TimeOfDay& start, const TimeOfDay& end) const
{
    uint32_t startMin = start.toMinutesOfDay();
    uint32_t endMin = end.toMinutesOfDay();

    if (endMin >= startMin)
    {
        return endMin - startMin;
    }
    // Midnight rollover case
    return (1440UL - startMin) + endMin;
}

void ScheduleEventEngine::checkMidnightRollover(const TimeOfDay& currentTime)
{
    if (firstRun)
    {
        return;
    }

    // Detected transition across midnight (e.g., from 23:59 to 00:00)
    if (currentTime.hour < lastEvaluatedTime.hour)
    {
        resetDailyTriggerFlags();

        DoseEvent resetEvent;
        resetEvent.eventId = ++nextEventId;
        resetEvent.type = EventType::DAILY_RESET;
        resetEvent.state = EventState::IDLE;
        resetEvent.triggeredTime = currentTime;
        notifyListeners(resetEvent);
    }
}

void ScheduleEventEngine::resetDailyTriggerFlags()
{
    for (uint8_t i = 0; i < cellCount; i++)
    {
        for (uint8_t j = 0; j < cells[i].scheduleCount; j++)
        {
            cells[i].schedules[j].triggeredToday = false;
        }
    }
}

void ScheduleEventEngine::checkSchedules(const TimeOfDay& currentTime)
{
    // Avoid evaluating trigger within the exact same minute unless first run
    bool isSameMinute = (!firstRun &&
                         currentTime.hour == lastEvaluatedTime.hour &&
                         currentTime.minute == lastEvaluatedTime.minute);

    if (isSameMinute)
    {
        return;
    }

    for (uint8_t i = 0; i < cellCount; i++)
    {
        if (cells[i].state != CellState::OCCUPIED || !cells[i].hasMedicine)
        {
            continue;
        }

        for (uint8_t j = 0; j < cells[i].scheduleCount; j++)
        {
            DoseSchedule& sched = cells[i].schedules[j];

            if (!sched.enabled || sched.triggeredToday)
            {
                continue;
            }

            if (currentTime.hour == sched.hour && currentTime.minute == sched.minute)
            {
                sched.triggeredToday = true;

                if (cells[i].currentStock == 0)
                {
                    // OUT OF STOCK: cannot dispense dose from empty cell!
                    DoseEvent emptyEvent;
                    emptyEvent.eventId = ++nextEventId;
                    emptyEvent.type = EventType::LOW_STOCK_WARNING;
                    emptyEvent.state = EventState::IDLE;
                    strncpy(emptyEvent.cellId, cells[i].cellId, MAX_ENGINE_ID_LEN - 1);
                    emptyEvent.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';
                    emptyEvent.cellIndex = i;
                    strncpy(emptyEvent.medicineId, cells[i].medicineId, MAX_ENGINE_ID_LEN - 1);
                    emptyEvent.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';
                    strncpy(emptyEvent.medicineName, cells[i].medicineName, MAX_ENGINE_STRING_LEN - 1);
                    emptyEvent.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';
                    emptyEvent.doseQuantity = 0; // Indicates 0 stock
                    emptyEvent.triggeredTime = currentTime;
                    emptyEvent.scheduledTime = TimeOfDay(sched.hour, sched.minute);
                    notifyListeners(emptyEvent);
                    continue;
                }

                if (dueEventCount < MAX_ACTIVE_EVENTS)
                {
                    DoseEvent event;
                    event.eventId = ++nextEventId;
                    event.type = EventType::DOSE_DUE;
                    event.state = EventState::DUE;

                    strncpy(event.cellId, cells[i].cellId, MAX_ENGINE_ID_LEN - 1);
                    event.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';
                    event.cellIndex = i;

                    strncpy(event.medicineId, cells[i].medicineId, MAX_ENGINE_ID_LEN - 1);
                    event.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';

                    strncpy(event.medicineName, cells[i].medicineName, MAX_ENGINE_STRING_LEN - 1);
                    event.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';

                    event.doseQuantity = sched.doseQuantity;
                    event.scheduledTime = TimeOfDay(sched.hour, sched.minute);
                    event.triggeredTime = currentTime;
                    event.triggeredEpoch = (timeProvider) ? timeProvider->getEpochSeconds() : 0;

                    activeDueEvents[dueEventCount++] = event;
                    notifyListeners(event);
                }
            }
        }
    }
}

void ScheduleEventEngine::checkGracePeriods(const TimeOfDay& currentTime)
{
    for (int16_t i = static_cast<int16_t>(dueEventCount) - 1; i >= 0; i--)
    {
        uint32_t elapsed = calculateElapsedMinutes(activeDueEvents[i].scheduledTime, currentTime);

        if (elapsed >= gracePeriodMinutes)
        {
            // Transition from DUE to MISSED
            DoseEvent missedEvent = activeDueEvents[i];
            missedEvent.type = EventType::DOSE_MISSED;
            missedEvent.state = EventState::MISSED;
            missedEvent.triggeredTime = currentTime;

            // Remove from due list
            removeDueEventByIndex(static_cast<uint8_t>(i));

            // Add to missed list
            if (missedEventCount < MAX_MISSED_EVENTS)
            {
                activeMissedEvents[missedEventCount++] = missedEvent;
            }

            notifyListeners(missedEvent);
        }
    }
}

void ScheduleEventEngine::checkLowStock()
{
    if (!lowStockWarningEnabled)
    {
        return;
    }

    for (uint8_t i = 0; i < cellCount; i++)
    {
        if (!cells[i].hasMedicine)
        {
            continue;
        }

        const uint16_t stock = cells[i].currentStock;

        if (stock <= cells[i].lowStockThreshold)
        {
            // Only fire once per stock level; avoids spamming every update() tick.
            if (stock == lastWarnedStock[i])
            {
                continue;
            }
            lastWarnedStock[i] = stock;

            DoseEvent warnEvent;
            warnEvent.eventId = ++nextEventId;
            warnEvent.type = EventType::LOW_STOCK_WARNING;
            warnEvent.state = EventState::IDLE;

            strncpy(warnEvent.cellId, cells[i].cellId, MAX_ENGINE_ID_LEN - 1);
            warnEvent.cellId[MAX_ENGINE_ID_LEN - 1] = '\0';
            warnEvent.cellIndex = i;

            strncpy(warnEvent.medicineId, cells[i].medicineId, MAX_ENGINE_ID_LEN - 1);
            warnEvent.medicineId[MAX_ENGINE_ID_LEN - 1] = '\0';

            strncpy(warnEvent.medicineName, cells[i].medicineName, MAX_ENGINE_STRING_LEN - 1);
            warnEvent.medicineName[MAX_ENGINE_STRING_LEN - 1] = '\0';

            warnEvent.doseQuantity = stock;
            warnEvent.triggeredTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

            notifyListeners(warnEvent);
        }
        else
        {
            // Stock is above threshold; reset guard so a future drop triggers again.
            lastWarnedStock[i] = 0xFFFFU;
        }
    }
}

void ScheduleEventEngine::update()
{
    if (!timeProvider || !timeProvider->isAvailable())
    {
        return;
    }

    TimeOfDay currentTime = timeProvider->getCurrentTime();

    checkMidnightRollover(currentTime);
    checkSchedules(currentTime);
    checkGracePeriods(currentTime);
    checkLowStock();   // Fires LOW_STOCK_WARNING when stock <= threshold (once per stock level)

    lastEvaluatedTime = currentTime;
    firstRun = false;
}

bool ScheduleEventEngine::confirmDose(uint8_t cellIndex)
{
    for (uint8_t i = 0; i < dueEventCount; i++)
    {
        if (activeDueEvents[i].cellIndex == cellIndex)
        {
            DoseEvent confirmedEvent = activeDueEvents[i];
            confirmedEvent.type = EventType::DOSE_CONFIRMED;
            confirmedEvent.state = EventState::CONFIRMED;
            confirmedEvent.resolvedTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

            // Decrement stock in cell (clamped at 0)
            if (cells[cellIndex].currentStock > 0)
            {
                if (cells[cellIndex].currentStock >= confirmedEvent.doseQuantity)
                {
                    cells[cellIndex].currentStock -= confirmedEvent.doseQuantity;
                }
                else
                {
                    cells[cellIndex].currentStock = 0;
                }
            }

            removeDueEventByIndex(i);
            notifyListeners(confirmedEvent);
            return true;
        }
    }
    return false;
}

bool ScheduleEventEngine::confirmDose(const char* cellId)
{
    int16_t index = findCellIndexById(cellId);
    if (index >= 0)
    {
        return confirmDose(static_cast<uint8_t>(index));
    }
    return false;
}

bool ScheduleEventEngine::dismissMissedDose(uint8_t cellIndex)
{
    for (uint8_t i = 0; i < missedEventCount; i++)
    {
        if (activeMissedEvents[i].cellIndex == cellIndex)
        {
            DoseEvent dismissedEvent = activeMissedEvents[i];
            dismissedEvent.type = EventType::DOSE_DISMISSED;
            dismissedEvent.state = EventState::DISMISSED;
            dismissedEvent.resolvedTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

            removeMissedEventByIndex(i);
            notifyListeners(dismissedEvent);
            return true;
        }
    }
    return false;
}

bool ScheduleEventEngine::dismissMissedDose(const char* cellId)
{
    int16_t index = findCellIndexById(cellId);
    if (index >= 0)
    {
        return dismissMissedDose(static_cast<uint8_t>(index));
    }
    return false;
}

uint8_t ScheduleEventEngine::dismissAllMissedDoses()
{
    uint8_t cleared = 0;
    while (missedEventCount > 0)
    {
        DoseEvent dismissedEvent = activeMissedEvents[0];
        dismissedEvent.type = EventType::DOSE_DISMISSED;
        dismissedEvent.state = EventState::DISMISSED;
        dismissedEvent.resolvedTime = (timeProvider) ? timeProvider->getCurrentTime() : TimeOfDay();

        removeMissedEventByIndex(0);
        notifyListeners(dismissedEvent);
        cleared++;
    }
    return cleared;
}

bool ScheduleEventEngine::handleCellButtonPress(uint8_t cellIndex)
{
    // Case 1: Active DUE dose for this cell -> Confirm it
    if (confirmDose(cellIndex))
    {
        return true;
    }

    // Case 2: Active MISSED dose for this cell -> Dismiss reminder
    if (dismissMissedDose(cellIndex))
    {
        return true;
    }

    // Case 3: Any button press clears all missed reminders (Mobile Clock reminder style)
    if (missedEventCount > 0)
    {
        dismissAllMissedDoses();
        return true;
    }

    return false;
}

bool ScheduleEventEngine::removeDueEventByIndex(uint8_t index)
{
    if (index >= dueEventCount)
    {
        return false;
    }
    for (uint8_t i = index; i < dueEventCount - 1; i++)
    {
        activeDueEvents[i] = activeDueEvents[i + 1];
    }
    dueEventCount--;
    return true;
}

bool ScheduleEventEngine::removeMissedEventByIndex(uint8_t index)
{
    if (index >= missedEventCount)
    {
        return false;
    }
    for (uint8_t i = index; i < missedEventCount - 1; i++)
    {
        activeMissedEvents[i] = activeMissedEvents[i + 1];
    }
    missedEventCount--;
    return true;
}

uint8_t ScheduleEventEngine::getDueEventCount() const
{
    return dueEventCount;
}

const DoseEvent* ScheduleEventEngine::getDueEvent(uint8_t index) const
{
    if (index >= dueEventCount)
    {
        return nullptr;
    }
    return &activeDueEvents[index];
}

const DoseEvent* ScheduleEventEngine::getDueEventForCell(uint8_t cellIndex) const
{
    for (uint8_t i = 0; i < dueEventCount; i++)
    {
        if (activeDueEvents[i].cellIndex == cellIndex)
        {
            return &activeDueEvents[i];
        }
    }
    return nullptr;
}

bool ScheduleEventEngine::hasActiveDueEvents() const
{
    return dueEventCount > 0;
}

bool ScheduleEventEngine::isCellDue(uint8_t cellIndex) const
{
    return getDueEventForCell(cellIndex) != nullptr;
}

uint8_t ScheduleEventEngine::getMissedEventCount() const
{
    return missedEventCount;
}

const DoseEvent* ScheduleEventEngine::getMissedEvent(uint8_t index) const
{
    if (index >= missedEventCount)
    {
        return nullptr;
    }
    return &activeMissedEvents[index];
}

const DoseEvent* ScheduleEventEngine::getMissedEventForCell(uint8_t cellIndex) const
{
    for (uint8_t i = 0; i < missedEventCount; i++)
    {
        if (activeMissedEvents[i].cellIndex == cellIndex)
        {
            return &activeMissedEvents[i];
        }
    }
    return nullptr;
}

bool ScheduleEventEngine::hasActiveMissedEvents() const
{
    return missedEventCount > 0;
}

bool ScheduleEventEngine::isCellMissed(uint8_t cellIndex) const
{
    return getMissedEventForCell(cellIndex) != nullptr;
}

bool ScheduleEventEngine::getNextScheduledDose(TimeOfDay& outTime, uint8_t& outCellIndex) const
{
    if (!timeProvider || !timeProvider->isAvailable())
    {
        return false;
    }

    TimeOfDay now = timeProvider->getCurrentTime();
    uint32_t currentMin = now.toMinutesOfDay();
    uint32_t minDiff = 1441; // Greater than 24 hours in minutes
    bool found = false;

    for (uint8_t i = 0; i < cellCount; i++)
    {
        if (cells[i].state != CellState::OCCUPIED || !cells[i].hasMedicine)
        {
            continue;
        }

        for (uint8_t j = 0; j < cells[i].scheduleCount; j++)
        {
            const DoseSchedule& sched = cells[i].schedules[j];
            if (!sched.enabled)
            {
                continue;
            }

            uint32_t schedMin = static_cast<uint32_t>(sched.hour) * 60UL + sched.minute;
            uint32_t diff = 0;

            if (schedMin > currentMin)
            {
                diff = schedMin - currentMin;
            }
            else
            {
                // Next day schedule
                diff = (1440UL - currentMin) + schedMin;
            }

            if (diff < minDiff)
            {
                minDiff = diff;
                outTime = TimeOfDay(sched.hour, sched.minute);
                outCellIndex = i;
                found = true;
            }
        }
    }
    return found;
}
