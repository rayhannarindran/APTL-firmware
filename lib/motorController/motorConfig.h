#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

// Pin definitions for motor control
#define STEP_PIN 23
#define DIR_PIN 22
#define ENABLE_PIN 21
#define LIMIT_PIN_TOP 33
// #define LIMIT_PIN_BOTTOM 32
// #define LIMIT_PIN_EMERGENCY 34 // Placed in middle of the vertical bar if hits unknown obstacle
#define MIN_SPEED 50
#define MAX_SPEED 70
#define MAX_POSITION_LIMIT 120.0 // mm

#define SERVO_LEFT_PIN 27
#define SERVO_MIDDLE_PIN 14
#define SERVO_RIGHT_PIN 13

#define SERVO_RELEASE_ANGLE 0
#define SERVO_PRESS_ANGLE 25
#define SERVO_PRESS_DURATION 700

#define PULLEY_TEETH 20.0
#define BELT_PITCH 2.0
#define MICROSTEPS 2
#define STEPS_PER_REV 200
#define STEPS_PER_MM (STEPS_PER_REV * MICROSTEPS / (PULLEY_TEETH * BELT_PITCH))
#define CLEARANCE_STEPS 50 // Steps to move away from limit switch after triggering it

#endif