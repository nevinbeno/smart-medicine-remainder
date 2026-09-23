#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

RTC_DS1307 rtc;

void setup() {

    Serial.begin(115200);

    // Start I2C
    Wire.begin(21, 22);

    // Start OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED initialization failed!");
        while (true);
    }

    // Start RTC
    if (!rtc.begin()) {
        Serial.println("RTC initialization failed!");
        while (true);
    }

    Serial.println("OLED initialized!");
    Serial.println("RTC initialized!");
}

void loop() {

    DateTime now = rtc.now();

    // -------------------------
    // Serial Monitor
    // -------------------------

    Serial.print("Time: ");

    if (now.hour() < 10)
        Serial.print("0");

    Serial.print(now.hour());
    Serial.print(":");

    if (now.minute() < 10)
        Serial.print("0");

    Serial.print(now.minute());
    Serial.print(":");

    if (now.second() < 10)
        Serial.print("0");

    Serial.println(now.second());


    // -------------------------
    // OLED
    // -------------------------

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SMART MEDICINE");

    display.setCursor(0, 15);
    display.println("CURRENT TIME");

    display.setTextSize(2);
    display.setCursor(10, 32);

    if (now.hour() < 10)
        display.print("0");

    display.print(now.hour());
    display.print(":");

    if (now.minute() < 10)
        display.print("0");

    display.print(now.minute());
    display.print(":");

    if (now.second() < 10)
        display.print("0");

    display.print(now.second());

    display.display();

    delay(1000);
}