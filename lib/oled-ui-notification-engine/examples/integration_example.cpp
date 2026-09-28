// This file is documentation/example code only; it is not the project's main.cpp.

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "schedule_event_engine.hpp"
#include "oled_notification_engine.hpp"

// The real project should create the scheduler's ITimeProvider and load
// RuntimeConfiguration through ConfigurationAdapter before normal operation.
// This example only shows the ownership boundary.

Adafruit_SSD1306 display(128, 64, &Wire, -1);
OledNotificationEngine oledUi;

// ScheduleEventEngine engine(&timeProvider);

void exampleSetup(ScheduleEventEngine& engine)
{
    Wire.begin(21, 22);
    oledUi.begin(display, engine);
}

void exampleLoop(ScheduleEventEngine& engine)
{
    engine.update();
    oledUi.update();
}
