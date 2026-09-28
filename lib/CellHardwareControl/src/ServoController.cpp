#include "ServoController.h"

ServoController::ServoController(int8_t pin)
    : _pin(pin),
      _currentAngle(0),
      _isAttached(false),
      _isOpen(false) {}

void ServoController::begin() {
    if (_pin >= 0 && !_isAttached) {
        // Pre-allocate hardware timers 1, 2, 3 for ESP32Servo
        // Leaving Timer 0 exclusively for tone() buzzer alert generation!
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);

        _servo.setPeriodHertz(50);        // Standard 50Hz servo frequency
        _servo.attach(_pin, 500, 2400);   // Standard 500us to 2400us limits for 0-180 deg
        _isAttached = true;
        close(0);
    }
}

void ServoController::writeAngle(uint8_t angle) {
    if (!_isAttached && _pin >= 0) {
        begin();
    }
    if (_isAttached) {
        if (angle > 180) angle = 180;
        _currentAngle = angle;
        _servo.write(angle);
    }
}

void ServoController::open(uint8_t angle) {
    writeAngle(angle);
    _isOpen = true;
}

void ServoController::close(uint8_t angle) {
    writeAngle(angle);
    _isOpen = false;
}

void ServoController::detach() {
    if (_isAttached) {
        _servo.detach();
        _isAttached = false;
    }
}
