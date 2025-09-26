#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "motorConfig.h"
#include "deviceConfig.h"

class MotorController {
public:
    MotorController();
    void setIdleTimeout(unsigned long ms);
    unsigned long getIdleTimeout() const { return idleTimeoutMs; }
    void setSpeed(int speed);
    int getSpeed() const { return (1000000 / (speedDelay * STEPS_PER_MM)) / 2; } // Returns speed in mm/s
    void setMaximumPosition(float max_y);
    float getMaximumPosition() const { return yMaximumPosition / STEPS_PER_MM; } // Returns max position in mm
    void setMaxPositionSetting(bool setting) { is_max_position_setting = setting; }
    bool getMaxPositionSetting() const { return is_max_position_setting; }
    float getCurrentPosition() const { return yPosition / STEPS_PER_MM; } // Returns current position in mm
    bool getMotorStatus() const { return !is_disabled; }

    void setup();
    long stepMotor(bool move_down, long steps);
    void disableMotor();
    bool calibrate();
    void moveTo(float y);
    void moveBy(float dy);
    void pressButton(int num_servo);
    void pressSpecificButton(int button);

    void checkIdle();
    void refreshIdle();

    void saveLineCoordinate(int line);

private:
    Servo servo_left;
    Servo servo_middle;
    Servo servo_right;

    int speedDelay;
    float yPosition;
    float yMaximumPosition;
    bool is_calibrated;
    bool is_emergency_stop;
    bool is_max_position_setting;
    bool is_disabled;

    volatile unsigned long lastActivityTimeMs;
    unsigned long idleTimeoutMs;
};

#endif