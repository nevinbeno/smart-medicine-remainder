#ifndef MOCK_ESP32_SERVO_H
#define MOCK_ESP32_SERVO_H

class ESP32PWM {
public:
    static void allocateTimer(int) {}
};

class Servo {
    int _pin = -1;
    int _angle = 0;
public:
    void setPeriodHertz(int) {}
    int attach(int p, int minUs = 500, int maxUs = 2400) { _pin = p; (void)minUs; (void)maxUs; return 1; }
    void write(int a) { _angle = a; }
    int read() const { return _angle; }
    bool attached() const { return _pin >= 0; }
    void detach() { _pin = -1; }
};

#endif // MOCK_ESP32_SERVO_H
