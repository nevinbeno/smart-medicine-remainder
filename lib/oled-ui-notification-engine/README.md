# OLED UI & Notification Engine

This is **Task 3** of the Smart Medicine Reminder project.

The module is intentionally packaged as a normal PlatformIO library under
`lib/`, so it requires **no root-level `extra_script.py` and no modification to
`platformio.ini`**.

## Responsibilities

The module owns only OLED presentation:

- current clock screen
- next scheduled dose
- active due-dose notifications
- simultaneous-dose rotation
- missed-dose notifications
- confirmation / dismissal status messages
- low-stock warning display

It does **not** own:

- schedules
- RTC hardware
- confirmation buttons
- LEDs
- buzzers
- stock decrementing
- medicine configuration

Those responsibilities stay in the other project modules.

## Folder placement

Copy this folder into the final project's `lib/` directory:

```text
project-root/
├── platformio.ini
├── src/
├── medicine-cell-data/
├── scheduling-event-engine/
└── lib/
    └── oled-ui-notification-engine/
        ├── include/
        │   └── oled_notification_engine.hpp
        ├── src/
        │   └── oled_notification_engine.cpp
        └── library.json
```

PlatformIO automatically discovers libraries under `lib/`.

## Integration

The final application creates the OLED display and scheduler because those
objects belong to the main integration layer:

```cpp
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "schedule_event_engine.hpp"
#include "oled_notification_engine.hpp"

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ScheduleEventEngine engine(&timeProvider);
OledNotificationEngine oledUi;

void setup()
{
    Wire.begin(21, 22);

    // The UI calls display.begin() itself.
    if (!oledUi.begin(display, engine))
    {
        Serial.println("OLED UI initialization failed");
    }

    // Load configuration into engine before normal operation.
    // ConfigurationAdapter::loadIntoEngine(configuration, engine);
}

void loop()
{
    engine.update();
    oledUi.update();
}
```

If the scheduler already has a different `ITimeProvider` setup, keep that
setup unchanged. The OLED module obtains the current time through
`engine.getTimeProvider()`.

## Event behaviour

### `DOSE_DUE`

The UI displays:

```text
MEDICINE DUE
Paracetamol
CELL CELL01  DOSE 1
TIME 08:00:00
TAKE MEDICINE
```

If multiple cells are due simultaneously, the UI rotates through all active
due events approximately every 1.8 seconds.

### `DOSE_MISSED`

The UI displays a persistent missed-dose screen. Because the scheduling
engine keeps the event in its missed-event list, the notification remains on
the OLED until the appropriate button action dismisses it.

If due and missed events coexist, the UI rotates through both sets of active
notifications.

### `DOSE_CONFIRMED`

A short confirmation screen is shown when there are no active due/missed
notifications. If another notification is active, the active notification
remains the display priority so that it cannot be hidden.

### `DOSE_DISMISSED`

A short `REMINDER CLEARED` message is shown when there are no active
notifications.

### `LOW_STOCK_WARNING`

A short warning screen displays the medicine, cell and reported stock value.
The scheduler currently defines the event, but its current `update()` path in
the supplied branch does not call `checkLowStock()`. The scheduler branch
should address that independently if low-stock events are required at runtime.

## Dynamic configuration

Nothing in this module assumes four cells or particular medicine names.

The scheduler supplies:

- cell ID
- cell index
- medicine name
- dose quantity
- scheduled time
- event state

Therefore the same UI can be used when the system changes from 1 cell to 4,
16 or 64 cells, subject to the scheduler's configured limits and OLED's
physical display size.

## No extra Python file

This module deliberately contains **no `extra_script.py`**. The earlier
version was unnecessary for the OLED component because PlatformIO can build
it automatically as a library under `lib/`.

Do not add this module's own `extra_scripts` entry to `platformio.ini`.

## Dependency

`library.json` declares:

- Adafruit GFX Library
- Adafruit SSD1306

The module's scheduler dependency is an internal project dependency and is
provided by the scheduling-event-engine branch after integration.
