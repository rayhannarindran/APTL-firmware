#include <Arduino.h>
#include <ESP32Servo.h>

#define LIMIT_SWITCH_PIN 33

#define STEP_PIN 23
#define DIR_PIN 22
#define ENABLE_PIN 21

//MOTOR CONFIG (TIMING BELT)
#define PULLEY_TEETH 20.0
#define BELT_PITCH 2.0
#define MICROSTEPS 2
#define STEPS_PER_REV 200
#define STEPS_PER_MM (STEPS_PER_REV * MICROSTEPS / (PULLEY_TEETH * BELT_PITCH))

int motorSpeed = 50; // mm/s
float testDistance = 20.0;

Servo myServoOne;
Servo myServoTwo;
Servo myServoThree;

void setup() {
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);

  myServoOne.attach(27);
  delay(1000);
  myServoTwo.attach(14);
  delay(1000);
  myServoThree.attach(13);
  delay(1000);

  myServoOne.write(90);
  myServoTwo.write(90);
  myServoThree.write(90);

  Serial.begin(115200);
}

void stepMotor(bool direction, long steps, long speedDelay) {
    digitalWrite(DIR_PIN, direction ? HIGH : LOW); // Set direction
    for (long i = 0; i < steps; i++) {
      
        if (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
            Serial.println("Limit switch PRESSED - Stopping motor");
            break;
        }

        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(speedDelay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(speedDelay);
    }
}

void loop() {
  // Move forward
  long steps = testDistance * STEPS_PER_MM;
  long speedDelay = (1000000.0 / (motorSpeed * STEPS_PER_MM)) / 2;
  stepMotor(true, steps, speedDelay);
  delay(1000);

  myServoOne.write(180);
  myServoTwo.write(180);
  myServoThree.write(180);
  delay(1000);

  myServoOne.write(90);
  myServoTwo.write(90);
  myServoThree.write(90);
  delay(1000);

  // Move backward
  stepMotor(false, steps, speedDelay);
  delay(1000);

  myServoOne.write(180);
  myServoTwo.write(180);
  myServoThree.write(180);
  delay(1000);

  myServoOne.write(90);
  myServoTwo.write(90);
  myServoThree.write(90);
  delay(1000);
}

//! SWITCH  ONLY TEST
// #define LIMIT_SWITCH_PIN 33

// void setup() {
//   pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);  // use ESP32 internal pull-up
//   Serial.begin(115200);
// }

// void loop() {
//   if (digitalRead(LIMIT_SWITCH_PIN) == LOW) {
//     Serial.println("Limit switch PRESSED");
//   } else {
//     Serial.println("Limit switch RELEASED");
//   }
//   delay(100);
// }