#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "../medicine-cell-data/include/configuration_loader.hpp"
#include "../scheduling-event-engine/include/configuration_adapter.hpp"
#include "../scheduling-event-engine/include/time_provider.hpp"

#include "oled_notification_engine.hpp"

// ============================================================
// OLED CONFIGURATION
// ============================================================

static constexpr uint8_t OLED_WIDTH = 128;
static constexpr uint8_t OLED_HEIGHT = 64;
static constexpr int8_t OLED_RESET = -1;
static constexpr uint8_t OLED_ADDRESS = 0x3C;


// ============================================================
// HARDWARE OBJECTS
// ============================================================

Adafruit_SSD1306 display(
    OLED_WIDTH,
    OLED_HEIGHT,
    &Wire,
    OLED_RESET
);

RTC_DS1307 rtc;


// ============================================================
// TIME + SCHEDULING ENGINE
// ============================================================

RTCTimeProvider timeProvider;

ScheduleEventEngine engine(&timeProvider);


// ============================================================
// OLED NOTIFICATION ENGINE
// ============================================================

// Rotate between multiple notifications every 1.8 seconds.
// Show temporary status screens for 1.2 seconds.
OledNotificationEngine oledUi(
    1800,
    1200
);


// ============================================================
// EMBEDDED configuration.json
// ============================================================

extern const uint8_t configuration_json_start[]
    asm("_binary_medicine_cell_data_config_configuration_json_start");

extern const uint8_t configuration_json_end[]
    asm("_binary_medicine_cell_data_config_configuration_json_end");


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" SMART MEDICINE REMINDER SYSTEM");
    Serial.println("========================================");


    // --------------------------------------------------------
    // 1. INITIALIZE I2C
    // --------------------------------------------------------

    Wire.begin(21, 22);

    Serial.println("[I2C] Initialized.");



    // --------------------------------------------------------
    // 2. INITIALIZE RTC
    // --------------------------------------------------------

    if (!rtc.begin())
    {
        Serial.println("[RTC] ERROR: RTC not found!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("[RTC] Initialized.");



    // --------------------------------------------------------
    // 3. LOAD EMBEDDED CONFIGURATION
    // --------------------------------------------------------

    const char* json =
        reinterpret_cast<const char*>(
            configuration_json_start
        );

    RuntimeConfiguration configuration;

    if (!loadConfiguration(json, configuration))
    {
        Serial.println(
            "[CONFIG] ERROR: Configuration loading failed!"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(
        "[CONFIG] Configuration loaded successfully."
    );

    Serial.print("[CONFIG] Cell count: ");
    Serial.println(configuration.cellCount);



    // --------------------------------------------------------
    // 4. LOAD CONFIGURATION INTO SCHEDULING ENGINE
    // --------------------------------------------------------

    if (!ConfigurationAdapter::loadIntoEngine(
            configuration,
            engine))
    {
        Serial.println(
            "[ENGINE] ERROR: Failed to load configuration "
            "into scheduling engine!"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.print("[ENGINE] Loaded cells: ");
    Serial.println(engine.getCellCount());



    // --------------------------------------------------------
    // 5. CONFIGURE SCHEDULING ENGINE
    // --------------------------------------------------------

    // Dose remains active for 5 minutes before becoming missed.
    engine.setGracePeriodMinutes(5);

    // Enable low-stock warning events.
    engine.setLowStockWarningEnabled(true);

    Serial.println(
        "[ENGINE] Grace period: 5 minutes."
    );

    Serial.println(
        "[ENGINE] Low-stock warnings: ENABLED."
    );



    // --------------------------------------------------------
    // 6. INITIALIZE OLED NOTIFICATION ENGINE
    // --------------------------------------------------------

    if (!oledUi.begin(display, engine))
    {
        Serial.println(
            "[OLED] ERROR: OLED initialization failed!"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(
        "[OLED] Initialized successfully."
    );



    // --------------------------------------------------------
    // 7. INITIAL ENGINE UPDATE
    // --------------------------------------------------------

    engine.update();
    oledUi.update();


    // --------------------------------------------------------
    // DONE
    // --------------------------------------------------------

    Serial.println();
    Serial.println("========================================");
    Serial.println(" SYSTEM INITIALIZATION COMPLETE");
    Serial.println("========================================");
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Update scheduling engine.
    //
    // This checks the RTC and generates events such as:
    //
    // DOSE_DUE
    // DOSE_MISSED
    // DOSE_CONFIRMED
    // DOSE_DISMISSED
    // DAILY_RESET
    // LOW_STOCK_WARNING
    // --------------------------------------------------------

    engine.update();


    // --------------------------------------------------------
    // Update OLED.
    //
    // OLED observes the scheduler's state/events and renders
    // the appropriate screen.
    // --------------------------------------------------------

    oledUi.update();


    // Small delay prevents the loop from running unnecessarily
    // fast while still keeping the UI responsive.
    delay(10);
}