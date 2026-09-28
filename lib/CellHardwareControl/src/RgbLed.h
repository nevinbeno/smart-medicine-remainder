#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include "CellHardwareTypes.h"

/**
 * @file RgbLed.h
 * @brief Driver for a discrete 3-channel RGB LED (Common Cathode / Common Anode).
 */
class RgbLed {
private:
    int8_t _pinR;
    int8_t _pinG;
    int8_t _pinB;
    bool _commonAnode;
    CellColor _currentColor;

    void applyChannels(bool r, bool g, bool b);

public:
    /**
     * @brief Construct an RGB LED controller.
     * @param pinR GPIO pin connected to Red anode/cathode.
     * @param pinG GPIO pin connected to Green anode/cathode.
     * @param pinB GPIO pin connected to Blue anode/cathode.
     * @param commonAnode True if common anode (LOW=ON), false if common cathode (HIGH=ON).
     */
    RgbLed(int8_t pinR = -1, int8_t pinG = -1, int8_t pinB = -1, bool commonAnode = false);

    /**
     * @brief Configure GPIO modes and initialize LED to OFF.
     */
    void begin();

    /**
     * @brief Set LED to a predefined logical color.
     */
    void setColor(CellColor color);

    /**
     * @brief Convenience helper: Turn LED RED.
     */
    void setRed();

    /**
     * @brief Convenience helper: Turn LED GREEN.
     */
    void setGreen();

    /**
     * @brief Convenience helper: Turn LED YELLOW (Red + Green).
     */
    void setYellow();

    /**
     * @brief Convenience helper: Turn LED BLUE.
     */
    void setBlue();

    /**
     * @brief Convenience helper: Turn LED OFF.
     */
    void setOff();

    /**
     * @brief Get the currently active logical color.
     */
    CellColor getCurrentColor() const;
};

#endif // RGB_LED_H
