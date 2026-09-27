#ifndef SCHEDULE_EVENT_ENGINE_HPP
#define SCHEDULE_EVENT_ENGINE_HPP

#include "time_engine_types.hpp"
#include "time_provider.hpp"

/**
 * @brief Scheduling and Time/Event Engine.
 * 
 * Central coordinator responsible for:
 * 1. Tracking real-time or simulated clock.
 * 2. Evaluating medicine schedules against current time.
 * 3. Detecting simultaneous doses across multiple cells.
 * 4. Managing event lifecycles: DUE -> CONFIRMED, or DUE -> MISSED -> DISMISSED.
 * 5. Notifying downstream modules (OLED UI, Hardware Alerts, Stock Manager).
 */
class ScheduleEventEngine
{
public:
    ScheduleEventEngine();
    explicit ScheduleEventEngine(ITimeProvider* provider);

    // ----------------------------------------------------
    // Time Provider & Configuration
    // ----------------------------------------------------
    void setTimeProvider(ITimeProvider* provider);
    ITimeProvider* getTimeProvider() const;

    void setGracePeriodMinutes(uint16_t minutes);
    uint16_t getGracePeriodMinutes() const;

    void setLowStockWarningEnabled(bool enabled);
    bool isLowStockWarningEnabled() const;

    // ----------------------------------------------------
    // Cell & Schedule Management
    // ----------------------------------------------------
    bool addCell(const EngineCell& cell);
    bool updateCell(uint8_t cellIndex, const EngineCell& cell);
    void clearCells();

    uint8_t getCellCount() const;
    const EngineCell* getCell(uint8_t cellIndex) const;
    EngineCell* getCellMutable(uint8_t cellIndex);
    int16_t findCellIndexById(const char* cellId) const;

    bool updateCellStock(uint8_t cellIndex, uint16_t newStock);

    // ----------------------------------------------------
    // Event Listeners / Callbacks
    // ----------------------------------------------------
    bool registerEventCallback(EventCallbackFn callback, void* userData = nullptr);
    bool unregisterEventCallback(EventCallbackFn callback);

    // ----------------------------------------------------
    // Main Processing Loop
    // ----------------------------------------------------
    /**
     * @brief Evaluates current time and updates all event states.
     * Must be called periodically (e.g., in Arduino loop() or timer tick).
     */
    void update();

    // ----------------------------------------------------
    // Dose Confirmation & Missed Handling
    // ----------------------------------------------------
    /**
     * @brief Confirms an active DUE dose for a cell.
     * Transitions event to EventState::CONFIRMED and emits EVENT_DOSE_CONFIRMED.
     * @param cellIndex Index of the cell whose confirm button was pressed.
     * @return true if an active DUE dose was found and confirmed.
     */
    bool confirmDose(uint8_t cellIndex);
    bool confirmDose(int cellIndex) { return confirmDose(static_cast<uint8_t>(cellIndex)); }
    bool confirmDose(const char* cellId);
    bool confirmDoseById(const char* cellId) { return confirmDose(cellId); }

    /**
     * @brief Dismisses a specific active missed dose notification.
     * Note: Does NOT decrement medicine stock.
     */
    bool dismissMissedDose(uint8_t cellIndex);
    bool dismissMissedDose(int cellIndex) { return dismissMissedDose(static_cast<uint8_t>(cellIndex)); }
    bool dismissMissedDose(const char* cellId);
    bool dismissMissedDoseById(const char* cellId) { return dismissMissedDose(cellId); }

    /**
     * @brief Dismisses all active missed dose notifications.
     * (e.g. When patient presses CONFIRM button on any cell to clear reminders).
     * @return Number of missed dose notifications cleared.
     */
    uint8_t dismissAllMissedDoses();

    /**
     * @brief Handles a button press on a cell:
     * - If cell has an active DUE dose: confirms it.
     * - If cell has an active MISSED dose: dismisses it without decrementing stock.
     * - If there are other MISSED doses and patient presses any button: dismisses missed notifications.
     */
    bool handleCellButtonPress(uint8_t cellIndex);

    // ----------------------------------------------------
    // Query APIs for OLED UI Engine & Alert Control
    // ----------------------------------------------------
    uint8_t getDueEventCount() const;
    const DoseEvent* getDueEvent(uint8_t index) const;
    const DoseEvent* getDueEventForCell(uint8_t cellIndex) const;
    bool hasActiveDueEvents() const;
    bool isCellDue(uint8_t cellIndex) const;

    uint8_t getMissedEventCount() const;
    const DoseEvent* getMissedEvent(uint8_t index) const;
    const DoseEvent* getMissedEventForCell(uint8_t cellIndex) const;
    bool hasActiveMissedEvents() const;
    bool isCellMissed(uint8_t cellIndex) const;

    /**
     * @brief Find the next scheduled dose across all cells relative to current time.
     */
    bool getNextScheduledDose(TimeOfDay& outTime, uint8_t& outCellIndex) const;

    /**
     * @brief Reset all daily triggered flags (typically called on midnight rollover).
     */
    void resetDailyTriggerFlags();

private:
    struct CallbackEntry
    {
        EventCallbackFn fn;
        void* userData;
    };

    ITimeProvider* timeProvider;
    uint16_t gracePeriodMinutes;
    bool lowStockWarningEnabled;

    EngineCell cells[MAX_ENGINE_CELLS];
    uint8_t cellCount;

    DoseEvent activeDueEvents[MAX_ACTIVE_EVENTS];
    uint8_t dueEventCount;

    DoseEvent activeMissedEvents[MAX_MISSED_EVENTS];
    uint8_t missedEventCount;

    CallbackEntry callbacks[MAX_EVENT_LISTENERS];
    uint8_t callbackCount;

    uint32_t nextEventId;
    TimeOfDay lastEvaluatedTime;
    bool firstRun;

    void notifyListeners(const DoseEvent& event);
    void checkSchedules(const TimeOfDay& currentTime);
    void checkGracePeriods(const TimeOfDay& currentTime);
    void checkLowStock();
    void checkMidnightRollover(const TimeOfDay& currentTime);

    bool removeDueEventByIndex(uint8_t index);
    bool removeMissedEventByIndex(uint8_t index);

    uint32_t calculateElapsedMinutes(const TimeOfDay& start, const TimeOfDay& end) const;
};

#endif // SCHEDULE_EVENT_ENGINE_HPP
