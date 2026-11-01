#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

static uint32_t g_mockMillis = 0;
inline uint32_t millis() { return g_mockMillis; }
inline void delay(uint32_t) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 1; }
inline void tone(int, int) {}
inline void noTone(int) {}
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int, int) {}
inline void ledcWriteTone(int, int) {}
inline void ledcWrite(int, int) {}

struct MockSerial {
    template<typename... Args>
    void printf(const char* fmt, Args... args) { ::printf(fmt, args...); }
    void println(const char* s = "") { ::printf("%s\n", s); }
    void print(const char* s) { ::printf("%s", s); }
};
static MockSerial Serial;

#endif // MOCK_ARDUINO_H
