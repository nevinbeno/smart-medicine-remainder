#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

/**
 * @file ServoController.h
 * @brief Dedicated servo actuator driver for an individual medicine compartment.
 *
 * Utilizes the ESP32Servo library with timer coordination and 50Hz PWM.
 */
class ServoController {
private:
    int8_t _pin;
    Servo _servo;
    uint8_t _currentAngle;
    bool _isAttached;
    bool _isOpen;

public:
    /**
     * @brief Construct a dedicated Servo Controller for a specific pin.
     * @param pin GPIO pin for PWM servo control.
     */
    ServoController(int8_t pin = -1);

    /**
     * @brief Allocate timer resources, configure PWM, and initialize servo to closed (0 degrees).
     */
    void begin();

    /**
     * @brief Move servo to specified angular position (0 - 180 degrees).
     * @param angle Target angle in degrees.
     */
    void writeAngle(uint8_t angle);

    /**
     * @brief Open compartment (default 90 degrees).
     */
    void open(uint8_t angle = 90);

    /**
     * @brief Close compartment (default 0 degrees).
     */
    void close(uint8_t angle = 0);

    /**
     * @brief Check if compartment is currently in open state.
     */
    bool isOpen() const { return _isOpen; }

    /**
     * @brief Get currently commanded angle.
     */
    uint8_t getCurrentAngle() const { return _currentAngle; }

    /**
     * @brief Detach PWM signal from pin.
     */
    void detach();
};

#endif // SERVO_CONTROLLER_H
