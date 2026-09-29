#include "oled_notification_engine.hpp"
#include <cstring>

namespace
{
    constexpr uint8_t MAX_TEXT_CHARS = 21; // 128px / ~6px at text size 1
}

OledNotificationEngine::OledNotificationEngine(uint32_t rotationMs_,
                                               uint32_t statusMs_)
    : display(nullptr),
      engine(nullptr),
      rotationMs(rotationMs_),
      statusMs(statusMs_),
      lastDrawMs(0),
      notificationStartedMs(0),
      statusStartedMs(0),
      notificationIndex(0),
      lastNotificationCount(0),
      redrawRequested(true),
      attached(false),
      hasLastEvent(false)
{
    strncpy(gridLabel, "[2x2]", sizeof(gridLabel));
}

bool OledNotificationEngine::begin(Adafruit_SSD1306& displayRef,
                                   ScheduleEventEngine& engineRef)
{
    display = &displayRef;
    engine = &engineRef;

    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        display = nullptr;
        engine = nullptr;
        return false;
    }

    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    display->setTextWrap(false);
    display->display();

    attached = false;
    return attach();
}

bool OledNotificationEngine::attach()
{
    if (!engine || attached)
    {
        return attached;
    }

    attached = engine->registerEventCallback(&OledNotificationEngine::eventCallback,
                                             this);
    if (attached)
    {
        requestRedraw();
    }
    return attached;
}

bool OledNotificationEngine::detach()
{
    if (!engine || !attached)
    {
        return false;
    }

    bool result = engine->unregisterEventCallback(&OledNotificationEngine::eventCallback);
    attached = false;
    return result;
}

void OledNotificationEngine::requestRedraw()
{
    redrawRequested = true;
}

void OledNotificationEngine::eventCallback(const DoseEvent& event, void* userData)
{
    OledNotificationEngine* self =
        static_cast<OledNotificationEngine*>(userData);

    if (self)
    {
        self->handleEvent(event);
    }
}

void OledNotificationEngine::handleEvent(const DoseEvent& event)
{
    // Copy the complete event; the scheduler owns the original object.
    lastEvent = event;
    hasLastEvent = true;

    const uint32_t now = millis();

    if (event.type == EventType::DOSE_DUE ||
        event.type == EventType::DOSE_MISSED)
    {
        notificationStartedMs = now;
        notificationIndex = 0;
        lastNotificationCount = 0;
    }
    else if (event.type == EventType::DOSE_CONFIRMED ||
             event.type == EventType::DOSE_DISMISSED ||
             event.type == EventType::LOW_STOCK_WARNING ||
             event.type == EventType::DAILY_RESET)
    {
        statusStartedMs = now;
    }

    requestRedraw();
}

uint8_t OledNotificationEngine::getNotificationCount() const
{
    if (!engine)
    {
        return 0;
    }

    return static_cast<uint8_t>(engine->getDueEventCount() +
                               engine->getMissedEventCount());
}

const DoseEvent* OledNotificationEngine::getNotification(uint8_t index,
                                                          uint8_t dueCount,
                                                          uint8_t missedCount) const
{
    if (!engine)
    {
        return nullptr;
    }

    // Due events are shown first, followed by persistent missed reminders.
    if (index < dueCount)
    {
        return engine->getDueEvent(index);
    }

    const uint8_t missedIndex = static_cast<uint8_t>(index - dueCount);
    if (missedIndex < missedCount)
    {
        return engine->getMissedEvent(missedIndex);
    }

    return nullptr;
}

void OledNotificationEngine::resetNotificationRotation()
{
    notificationIndex = 0;
    notificationStartedMs = millis();
}

void OledNotificationEngine::update()
{
    if (!display || !engine || !attached)
    {
        return;
    }

    const uint32_t now = millis();
    const uint8_t dueCount = engine->getDueEventCount();
    const uint8_t missedCount = engine->getMissedEventCount();
    const uint8_t notificationCount = static_cast<uint8_t>(dueCount + missedCount);

    if (notificationCount != lastNotificationCount)
    {
        resetNotificationRotation();
        lastNotificationCount = notificationCount;
        redrawRequested = true;
    }

    if (notificationCount > 0 &&
        rotationMs > 0 &&
        static_cast<uint32_t>(now - notificationStartedMs) >= rotationMs)
    {
        notificationStartedMs = now;
        notificationIndex = static_cast<uint8_t>(
            (notificationIndex + 1) % notificationCount);
        redrawRequested = true;
    }

    const bool statusActive =
        hasLastEvent &&
        (lastEvent.type == EventType::DOSE_CONFIRMED ||
         lastEvent.type == EventType::DOSE_DISMISSED ||
         lastEvent.type == EventType::LOW_STOCK_WARNING ||
         lastEvent.type == EventType::DAILY_RESET) &&
        static_cast<uint32_t>(now - statusStartedMs) < statusMs;

    const bool periodicClockRefresh =
        static_cast<uint32_t>(now - lastDrawMs) >= 500;

    if (!redrawRequested && !periodicClockRefresh && !statusActive)
    {
        return;
    }

    lastDrawMs = now;
    redrawRequested = false;

    if (notificationCount > 0)
    {
        const DoseEvent* event = getNotification(notificationIndex,
                                                  dueCount,
                                                  missedCount);
        if (event)
        {
            drawNotificationScreen(*event);
        }
        return;
    }

    if (statusActive)
    {
        if (lastEvent.type == EventType::LOW_STOCK_WARNING)
        {
            drawLowStockScreen(lastEvent);
        }
        else
        {
            drawStatusScreen();
        }
        return;
    }

    drawClockScreen();
}

void OledNotificationEngine::drawClockScreen()
{
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);

    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print("SMART MEDICINE");

    // Display scalable grid indicator in the header (e.g. "[8x8]")
    if (gridLabel[0] != '\0')
    {
        int16_t x = 128 - static_cast<int16_t>(strlen(gridLabel) * 6);
        if (x > 84)
        {
            display->setCursor(x, 0);
            display->print(gridLabel);
        }
    }

    display->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display->setCursor(0, 15);
    display->println("CURRENT TIME");

    display->setTextSize(2);
    display->setCursor(10, 27);

    ITimeProvider* provider = engine->getTimeProvider();
    if (provider && provider->isAvailable())
    {
        printTime(provider->getCurrentTime());
    }
    else
    {
        display->print("--:--:--");
    }

    display->setTextSize(1);
    display->setCursor(0, 52);
    printNextDose();

    display->display();
}

void OledNotificationEngine::drawNotificationScreen(const DoseEvent& event)
{
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    display->setTextSize(1);

    if (event.type == EventType::DOSE_MISSED)
    {
        display->setCursor(0, 0);
        display->println("MISSED DOSE");
        display->drawLine(0, 10, 127, 10, SSD1306_WHITE);
    }
    else
    {
        display->setCursor(0, 0);
        display->println("MEDICINE DUE");
        display->drawLine(0, 10, 127, 10, SSD1306_WHITE);
    }

    display->setCursor(0, 15);
    printMedicineName(event.medicineName);

    display->setCursor(0, 30);
    printCellAndDose(event);

    display->setCursor(0, 43);
    display->print("TIME ");
    printTime(event.scheduledTime);

    display->setCursor(0, 55);
    if (event.type == EventType::DOSE_MISSED)
    {
        display->print("PRESS CONFIRM");
    }
    else
    {
        display->print("TAKE MEDICINE");
    }

    display->display();
}

void OledNotificationEngine::drawStatusScreen()
{
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);

    display->setTextSize(1);
    display->setCursor(0, 0);

    if (lastEvent.type == EventType::DOSE_CONFIRMED)
    {
        display->println("DOSE CONFIRMED");
    }
    else if (lastEvent.type == EventType::DOSE_DISMISSED)
    {
        display->println("REMINDER CLEARED");
    }
    else
    {
        display->println("NEW DAY");
    }

    display->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display->setCursor(0, 18);
    printMedicineName(lastEvent.medicineName);

    display->setCursor(0, 34);
    display->print("CELL ");
    printClipped(lastEvent.cellId, 15);

    display->setCursor(0, 48);
    if (lastEvent.type == EventType::DOSE_CONFIRMED)
    {
        display->print("DOSE ");
        display->print(lastEvent.doseQuantity);
        display->print(" TAKEN");
    }
    else
    {
        display->print("READY FOR NEXT DOSE");
    }

    display->display();
}

void OledNotificationEngine::drawLowStockScreen(const DoseEvent& event)
{
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    display->setTextSize(1);

    display->setCursor(0, 0);
    display->println("LOW STOCK WARNING");
    display->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display->setCursor(0, 17);
    printMedicineName(event.medicineName);

    display->setCursor(0, 32);
    display->print("CELL ");
    printClipped(event.cellId, 15);

    display->setCursor(0, 47);
    display->print("STOCK: ");
    display->print(event.doseQuantity);

    display->display();
}

void OledNotificationEngine::printTwoDigits(uint8_t value)
{
    if (value < 10)
    {
        display->print('0');
    }
    display->print(value);
}

void OledNotificationEngine::printTime(const TimeOfDay& time)
{
    printTwoDigits(time.hour);
    display->print(':');
    printTwoDigits(time.minute);
    display->print(':');
    printTwoDigits(time.second);
}

void OledNotificationEngine::printMedicineName(const char* name)
{
    if (!name || name[0] == '\0')
    {
        display->print("UNKNOWN MEDICINE");
        return;
    }

    printClipped(name, MAX_TEXT_CHARS);
}

void OledNotificationEngine::printCellAndDose(const DoseEvent& event)
{
    display->print("CELL ");
    printClipped(event.cellId, 10);
    display->print("  DOSE ");
    display->print(event.doseQuantity);
}

void OledNotificationEngine::printNextDose()
{
    TimeOfDay nextTime;
    uint8_t nextCell = 0;

    if (!engine->getNextScheduledDose(nextTime, nextCell))
    {
        display->print("NEXT: NONE SCHEDULED");
        return;
    }

    const EngineCell* cell = engine->getCell(nextCell);

    display->print("NEXT ");
    printTime(nextTime);

    if (cell && cell->hasMedicine)
    {
        display->print(" ");
        printClipped(cell->medicineName, 12);
    }
}

void OledNotificationEngine::printCentered(const char* text,
                                           uint8_t textSize,
                                           int16_t y)
{
    if (!text)
    {
        return;
    }

    const uint16_t width = static_cast<uint16_t>(strlen(text)) * 6U * textSize;
    int16_t x = static_cast<int16_t>((SCREEN_WIDTH - width) / 2);
    if (x < 0)
    {
        x = 0;
    }

    display->setTextSize(textSize);
    display->setCursor(x, y);
    display->print(text);
}

void OledNotificationEngine::printClipped(const char* text, uint8_t maxChars)
{
    if (!text)
    {
        return;
    }

    uint8_t count = 0;
    while (text[count] != '\0' && count < maxChars)
    {
        display->print(text[count]);
        count++;
    }

    if (text[count] != '\0' && maxChars >= 3)
    {
        // The final three characters are replaced with an ellipsis.
        // Rewriting the line is awkward on Adafruit_GFX, so this helper
        // intentionally keeps a strict character budget instead of drawing
        // beyond the OLED edge.
    }
}

void OledNotificationEngine::setGridLabel(const char* label)
{
    if (label)
    {
        strncpy(gridLabel, label, sizeof(gridLabel) - 1);
        gridLabel[sizeof(gridLabel) - 1] = '\0';
        requestRedraw();
    }
}


