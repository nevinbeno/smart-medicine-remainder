#ifndef OLED_NOTIFICATION_ENGINE_HPP
#define OLED_NOTIFICATION_ENGINE_HPP

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "schedule_event_engine.hpp"

/**
 * OLED UI + notification renderer for the Smart Medicine Reminder system.
 *
 * Design rules:
 *  - The scheduling/event engine owns schedules and event state.
 *  - This module only observes the engine and renders the UI.
 *  - Cell count, medicine names, times and quantities are never hard-coded.
 *  - Button/LED/buzzer/stock actions remain in their respective modules.
 *  - update() is non-blocking; it never uses delay().
 */
class OledNotificationEngine
{
public:
    static constexpr uint8_t SCREEN_WIDTH = 128;
    static constexpr uint8_t SCREEN_HEIGHT = 64;

    explicit OledNotificationEngine(uint32_t rotationMs = 1800,
                                    uint32_t statusMs = 1200);

    /**
     * Attach the already-created Adafruit_SSD1306 display and scheduler.
     * The scheduler must outlive this object.
     */
    bool begin(Adafruit_SSD1306& display, ScheduleEventEngine& engine);

    /** Register/unregister this module's scheduler event listener. */
    bool attach();
    bool detach();

    /** Call frequently from Arduino loop(); no delay() is used. */
    void update();

    /** Force the next update to redraw immediately. */
    void requestRedraw();

    bool isAttached() const { return attached; }

private:
    Adafruit_SSD1306* display;
    ScheduleEventEngine* engine;

    uint32_t rotationMs;
    uint32_t statusMs;
    uint32_t lastDrawMs;
    uint32_t notificationStartedMs;
    uint32_t statusStartedMs;
    uint8_t notificationIndex;
    uint8_t lastNotificationCount;
    bool redrawRequested;
    bool attached;

    // Last event is copied because scheduler callback arguments are temporary.
    DoseEvent lastEvent;
    bool hasLastEvent;

    static void eventCallback(const DoseEvent& event, void* userData);
    void handleEvent(const DoseEvent& event);

    void drawClockScreen();
    void drawNotificationScreen(const DoseEvent& event);
    void drawStatusScreen();
    void drawLowStockScreen(const DoseEvent& event);

    const DoseEvent* getNotification(uint8_t index, uint8_t dueCount,
                                     uint8_t missedCount) const;

    uint8_t getNotificationCount() const;
    void resetNotificationRotation();

    void printTwoDigits(uint8_t value);
    void printTime(const TimeOfDay& time);
    void printMedicineName(const char* name);
    void printCellAndDose(const DoseEvent& event);
    void printNextDose();
    void printCentered(const char* text, uint8_t textSize, int16_t y);
    void printClipped(const char* text, uint8_t maxChars);
};

#endif // OLED_NOTIFICATION_ENGINE_HPP
