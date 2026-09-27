# Scheduling & Time/Event Engine API Reference

## Table of Contents
1. [Core Data Types](#1-core-data-types)
2. [ScheduleEventEngine Class](#2-scheduleeventengine-class)
3. [ConfigurationAdapter Class](#3-configurationadapter-class)
4. [ITimeProvider Interface](#4-itimeprovider-interface)
5. [Integration Guide for Teammates](#5-integration-guide-for-teammates)

---

## 1. Core Data Types

### `EventType`
```cpp
enum class EventType
{
    NONE = 0,
    DOSE_DUE,            // Scheduled time reached; hardware alert active
    DOSE_MISSED,         // Grace period expired without confirmation
    DOSE_CONFIRMED,      // User pressed confirm button for cell
    DOSE_DISMISSED,      // User dismissed missed reminder
    DAILY_RESET,         // Midnight rollover occurred
    LOW_STOCK_WARNING    // Stock level dropped below threshold
};
```

### `EventState`
```cpp
enum class EventState
{
    IDLE = 0,            // Scheduled, pending arrival
    DUE,                 // Active alarm state
    MISSED,              // Overdue notification state
    CONFIRMED,           // Taken successfully
    DISMISSED            // Acknowledged and cleared
};
```

### `TimeOfDay`
```cpp
struct TimeOfDay
{
    uint8_t hour;        // 0-23
    uint8_t minute;      // 0-59
    uint8_t second;      // 0-59

    uint32_t toSecondsOfDay() const;
    uint32_t toMinutesOfDay() const;
    bool operator==(const TimeOfDay& other) const;
    bool operator<(const TimeOfDay& other) const;
    bool isValid() const;
};
```

### `DoseEvent`
```cpp
struct DoseEvent
{
    uint32_t eventId;
    EventType type;
    EventState state;

    char cellId[16];
    uint8_t cellIndex;

    char medicineId[16];
    char medicineName[32];
    uint16_t doseQuantity;

    TimeOfDay scheduledTime;
    TimeOfDay triggeredTime;
    TimeOfDay resolvedTime;
    uint32_t triggeredEpoch;
};
```

### `EventCallbackFn`
```cpp
typedef void (*EventCallbackFn)(const DoseEvent& event, void* userData);
```

---

## 2. ScheduleEventEngine Class

### Initialization & Configuration
```cpp
ScheduleEventEngine();
explicit ScheduleEventEngine(ITimeProvider* provider);

void setTimeProvider(ITimeProvider* provider);
ITimeProvider* getTimeProvider() const;

void setGracePeriodMinutes(uint16_t minutes);
uint16_t getGracePeriodMinutes() const;

void setLowStockWarningEnabled(bool enabled);
bool isLowStockWarningEnabled() const;
```

### Cell Management
```cpp
bool addCell(const EngineCell& cell);
bool updateCell(uint8_t cellIndex, const EngineCell& cell);
void clearCells();
uint8_t getCellCount() const;
const EngineCell* getCell(uint8_t cellIndex) const;
EngineCell* getCellMutable(uint8_t cellIndex);
int16_t findCellIndexById(const char* cellId) const;
bool updateCellStock(uint8_t cellIndex, uint16_t newStock);
```

### Callback Subscription
```cpp
bool registerEventCallback(EventCallbackFn callback, void* userData = nullptr);
bool unregisterEventCallback(EventCallbackFn callback);
```

### Execution Loop
```cpp
void update(); // Call inside Arduino loop()
```

### Dose Confirmation & Missed Notification Handling
```cpp
bool confirmDose(uint8_t cellIndex);
bool confirmDose(const char* cellId);

bool dismissMissedDose(uint8_t cellIndex);
bool dismissMissedDose(const char* cellId);

uint8_t dismissAllMissedDoses();

bool handleCellButtonPress(uint8_t cellIndex);
```

### Query Methods (for OLED UI & Alert Hardware)
```cpp
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

bool getNextScheduledDose(TimeOfDay& outTime, uint8_t& outCellIndex) const;
void resetDailyTriggerFlags();
```

---

## 3. ConfigurationAdapter Class

```cpp
class ConfigurationAdapter
{
public:
    static bool loadIntoEngine(const RuntimeConfiguration& config, ScheduleEventEngine& engine);
    static EngineCell convertCell(const RuntimeCell& sourceCell, uint8_t cellIndex);
};
```

---

## 4. ITimeProvider Interface

```cpp
class ITimeProvider
{
public:
    virtual ~ITimeProvider() = default;
    virtual TimeOfDay getCurrentTime() = 0;
    virtual uint32_t getEpochSeconds() = 0;
    virtual bool isAvailable() = 0;
};
```

---

## 5. Integration Guide for Teammates

### For Teammate 3: OLED UI & Notification Engine
- Register an event callback with `registerEventCallback()` to receive immediate triggers.
- In your display loop:
  - If `hasActiveDueEvents()`:
    - Iterate `getDueEvent(i)` for $i \in [0, \text{dueCount}-1]$.
    - Roll / scroll the medicine names, cell IDs, and dose quantities on screen.
  - If `hasActiveMissedEvents()`:
    - Display persistent reminder banner (e.g. "MISSED: Paracetamol 08:00").
  - Otherwise:
    - Display current clock time and upcoming dose from `getNextScheduledDose()`.

### For Teammate 4: Cell Hardware & Alert Control
- On `DOSE_DUE`:
  - Turn corresponding compartment RGB LED to **RED**.
  - Turn common **Alert Buzzer** ON (beeping pattern).
- On `DOSE_CONFIRMED`:
  - Turn compartment RGB LED to **GREEN** for 1 second.
  - If no more due events (`!hasActiveDueEvents()`), turn Alert Buzzer OFF.
- On `DOSE_MISSED`:
  - Turn Alert Buzzer OFF (silent reminder mode).
- On `LOW_STOCK_WARNING`:
  - Turn compartment RGB LED to **YELLOW**.
  - Beep **Warning Buzzer** (distinct low-stock chirp).

### For Teammate 5: Dose, Stock & Confirmation Management
- When user presses a cell confirmation button:
  - Call `engine.handleCellButtonPress(cellIndex)`.
- In your event callback:
  - If `event.type == EventType::DOSE_CONFIRMED`:
    - Decrement cell stock by `event.doseQuantity`.
    - Log dose history record.
  - If `event.type == EventType::DOSE_DISMISSED`:
    - Do NOT decrement stock. Log missed dose reminder cleared.
