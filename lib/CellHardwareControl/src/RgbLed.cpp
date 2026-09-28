#include "RgbLed.h"

RgbLed::RgbLed(int8_t pinR, int8_t pinG, int8_t pinB, bool commonAnode)
    : _pinR(pinR),
      _pinG(pinG),
      _pinB(pinB),
      _commonAnode(commonAnode),
      _currentColor(COLOR_OFF) {}

void RgbLed::begin() {
    if (_pinR >= 0) pinMode(_pinR, OUTPUT);
    if (_pinG >= 0) pinMode(_pinG, OUTPUT);
    if (_pinB >= 0) pinMode(_pinB, OUTPUT);
    setOff();
}

void RgbLed::applyChannels(bool r, bool g, bool b) {
    // For Common Cathode: HIGH turns channel ON, LOW turns channel OFF.
    // For Common Anode:   LOW turns channel ON, HIGH turns channel OFF.
    uint8_t levelR = _commonAnode ? (r ? LOW : HIGH) : (r ? HIGH : LOW);
    uint8_t levelG = _commonAnode ? (g ? LOW : HIGH) : (g ? HIGH : LOW);
    uint8_t levelB = _commonAnode ? (b ? LOW : HIGH) : (b ? HIGH : LOW);

    if (_pinR >= 0) digitalWrite(_pinR, levelR);
    if (_pinG >= 0) digitalWrite(_pinG, levelG);
    if (_pinB >= 0) digitalWrite(_pinB, levelB);
}

void RgbLed::setColor(CellColor color) {
    _currentColor = color;
    switch (color) {
        case COLOR_RED:
            applyChannels(true, false, false);
            break;
        case COLOR_GREEN:
            applyChannels(false, true, false);
            break;
        case COLOR_YELLOW:
            // Additive color mixing: Red + Green = Yellow
            applyChannels(true, true, false);
            break;
        case COLOR_BLUE:
            applyChannels(false, false, true);
            break;
        case COLOR_WHITE:
            applyChannels(true, true, true);
            break;
        case COLOR_OFF:
        default:
            applyChannels(false, false, false);
            break;
    }
}

void RgbLed::setRed() {
    setColor(COLOR_RED);
}

void RgbLed::setGreen() {
    setColor(COLOR_GREEN);
}

void RgbLed::setYellow() {
    setColor(COLOR_YELLOW);
}

void RgbLed::setBlue() {
    setColor(COLOR_BLUE);
}

void RgbLed::setOff() {
    setColor(COLOR_OFF);
}

CellColor RgbLed::getCurrentColor() const {
    return _currentColor;
}
