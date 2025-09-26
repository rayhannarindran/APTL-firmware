#include "motorController.h"

MotorController::MotorController() 
    : yPosition(0), yMaximumPosition(0), speedDelay(500), 
    is_calibrated(false),  is_emergency_stop(false), is_max_position_setting(false), is_disabled(false),
    idleTimeoutMs(1 * 60 * 1000), lastActivityTimeMs(millis()) { //5 Minutes Default
}

void MotorController::setSpeed(int speed) {
    Serial.println("Setting speed to: " + String(speed) + " mm/s");

    if (speed < MIN_SPEED || speed > MAX_SPEED) {
        Serial.println("Speed out of bounds. Please set a value between " + String(MIN_SPEED) + " and " + String(MAX_SPEED) + ".\n");
        return;
    }

    speedDelay = (1000000.0 / (speed * STEPS_PER_MM)) / 2; // Convert speed to delay in microseconds
    Serial.println("Speed set to: " + String(speed) + " mm/s\n");
}

void MotorController::setMaximumPosition(float max_y) {
    if (!getMaxPositionSetting()) {
        Serial.println("Error: Maximum position setting is not enabled. Enable it first to set maximum position.\n");
        return;
    }

    Serial.println("Setting maximum position to: " + String(max_y) + " mm");

    if (max_y <= 0 || max_y > MAX_POSITION_LIMIT) {
        Serial.println("Maximum position must be greater than 0 and less than " + String(MAX_POSITION_LIMIT) + ".\n");
        return;
    }

    yMaximumPosition = lround(max_y * STEPS_PER_MM);
    setMaxPosition(max_y);
    Serial.println("Maximum position set to: " + String(max_y) + " mm\n");
}


void MotorController::setup() {
    Serial.println("Setting up motor and servos...\n");

    // Motor pin setup
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(ENABLE_PIN, LOW);
    setSpeed(MIN_SPEED); // Set initial speed to minimum

    // Limit switch pin setup
    pinMode(LIMIT_PIN_TOP, INPUT_PULLUP);
    // pinMode(LIMIT_PIN_BOTTOM, INPUT_PULLUP);
    // pinMode(LIMIT_PIN_EMERGENCY, INPUT_PULLUP);
 
    // Servo pin setup
    servo_left.attach(SERVO_LEFT_PIN);
    servo_middle.attach(SERVO_MIDDLE_PIN);
    servo_right.attach(SERVO_RIGHT_PIN);

    servo_left.write(SERVO_RELEASE_ANGLE);
    servo_middle.write(SERVO_RELEASE_ANGLE);
    servo_right.write(SERVO_RELEASE_ANGLE);

    Serial.println("Motor and servos setup complete.\n");
}


//* STEPPER FUNCTIONS
bool MotorController::calibrate() {
    Serial.println("Calibrating tool position...");

    // Move to the top limit
    digitalWrite(DIR_PIN, LOW);
    while (digitalRead(LIMIT_PIN_TOP) == HIGH) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(speedDelay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(speedDelay);
    }
    stepMotor(true, CLEARANCE_STEPS);
    yPosition = 0;
    is_calibrated = true;
    delay(1000);

    Serial.println("Calibration complete\n");
    return true;
}

void MotorController::moveTo(float y) {
    refreshIdle();

    if (isnan(y) || isinf(y)) {
        Serial.println("Invalid target position.\n");
        return;
    }

    if (!is_calibrated) {
        Serial.println("Motor not calibrated. Please calibrating.\n");
        bool calibrateSuccess = calibrate();
        if (!calibrateSuccess) {
            Serial.println("Calibration failed. Aborting move.\n");
            return;
        }
        Serial.println("Calibration successful. Continuing move.");
    }

    if (is_emergency_stop) {
        Serial.println("Emergency stop is active. Cannot move.\n");
        return;
    }

    if (yMaximumPosition == 0) {
        Serial.println("Maximum position not set. Please set it first.\n");
        return;
    }

    Serial.println("Moving to position: " + String(y));

    long yTargetPosition = lround(y * STEPS_PER_MM); //! LOOKAT THE CONVERSION FACTOR IN motorConfig.h

    if (yTargetPosition < 0 || yTargetPosition > yMaximumPosition || yTargetPosition > (MAX_POSITION_LIMIT * STEPS_PER_MM)) {
        Serial.print("Target position out of bounds. yTarget: ");
        return;
    }

    long steps = yTargetPosition - yPosition;
    if (steps == 0) {
        Serial.println("Already at target position.\n");
        return;
    }
    long moved = stepMotor(steps > 0, abs(steps));
    if (moved < abs(steps)) {
        Serial.println("Warning: Movement was limited by emergency stop or limit switch.\n");
    }

    Serial.println("Moved to position: " + String(y) + "\n");
}

void MotorController::moveBy(float dy) {
    refreshIdle();

    if (isnan(dy) || isinf(dy)) {
        Serial.println("Invalid movement value.\n");
        return;
    }

    Serial.println("Moving by: " + String(dy) + " mm");

    if (!is_calibrated) {
        Serial.println("Tool not calibrated. Calibrating first.");
        float tempCurrentPosition = getCurrentPosition();
        bool calibrateSuccess = calibrate();
        if (!calibrateSuccess) {
            Serial.println("Calibration failed. Aborting move.\n");
            return;
        }
        Serial.println("Calibration successful. Continuing move from latest position.");
        moveTo(tempCurrentPosition); // Move back to the original position after calibration
    }

    if (is_emergency_stop) {
        Serial.println("Emergency stop is active. Cannot move.\n");
        return;
    }

    // if (yMaximumPosition == 0 && !is_max_position_setting) {
    //     Serial.println("Maximum position not set. Please set it first.\n");
    //     return;
    // }

    // if (is_max_position_setting) {
    //     Serial.println("WARNING: Currently setting maximum position. Be careful when moving the tool manually.\n");
    // }

    long yTargetPosition = yPosition + lround(dy * STEPS_PER_MM); //! LOOKAT THE CONVERSION FACTOR IN motorConfig.h

    // if (yTargetPosition < 0 || yTargetPosition > yMaximumPosition && !is_max_position_setting || yTargetPosition > MAX_POSITION_LIMIT) {
    //     Serial.println("Target position out of bounds.\n");
    //     return;
    // }

    if (yTargetPosition < 0 || yTargetPosition > (MAX_POSITION_LIMIT * STEPS_PER_MM)) {
        Serial.println("Target position out of bounds.\n");
        return;
    }

    long steps = yTargetPosition - yPosition;
    if (steps == 0) {
        Serial.println("Already at target position.\n");
        return;
    }
    long moved = stepMotor(steps > 0, abs(steps));
    if (moved < abs(steps)) {
        Serial.println("Warning: Movement was limited by emergency stop or limit switch.\n");
    }

    Serial.println("Moved by: " + String(dy) + "\n");
}

long MotorController::stepMotor(bool move_down, long steps) {
    refreshIdle();

    if (is_emergency_stop) {
        Serial.println("Emergency stop is active. Cannot move motor.\n");
        return 0;
    }

    if (digitalRead(LIMIT_PIN_TOP) == LOW && !move_down) {
        Serial.println("Top limit switch triggered. Cannot move up.\n");
        return 0;
    }

    if (steps <= 0) {
        Serial.println("No steps to move.\n");
        return 0;
    }

    digitalWrite(ENABLE_PIN, LOW);
    digitalWrite(DIR_PIN, move_down ? HIGH : LOW); // Set direction

    long moved = 0;
    for (long i = 0; i < steps; ++i) {
        // abort checks inside loop
        if (is_emergency_stop) {
            Serial.println("Emergency stop detected - halting.\n");
            break;
        }
        if (digitalRead(LIMIT_PIN_TOP) == LOW && !move_down) {
            Serial.println("Top limit reached - stopping.\n");
            break;
        }

        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(speedDelay);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(speedDelay);

        ++moved;
        // occasionally refresh to avoid idle timeout during long moves
        if ((moved & 0xFF) == 0) refreshIdle();
    }

    // Update yPosition according to actual moved steps
    if (moved > 0) {
        if (move_down) {
            yPosition += moved;

            // if (yMaximumPosition > 0 && yPosition > yMaximumPosition) {
            //     yPosition = yMaximumPosition;
            // }
        } else {
            yPosition -= moved;

            // long minPosition = CLEARANCE_STEPS;
            // if (yPosition < minPosition) {
            //     yPosition = minPosition;
            // }
        }
    }

    delay(50);
    return moved;
}

void MotorController::disableMotor() {
    if (is_disabled) return;
    digitalWrite(ENABLE_PIN, HIGH); // Disable motor driver
    is_disabled = true;
    is_calibrated = false;
    Serial.println("Motor disabled.\n");
}


//* SERVO FUNCTIONS
void MotorController::pressButton(int num_servo) {
    refreshIdle();

    Serial.println("Pressing servo: " + String(num_servo));

    if (!is_calibrated) {
        Serial.println("Tool not calibrated. Please calibrate first.\n");
        return;
    }

    if (is_emergency_stop) {
        Serial.println("Emergency stop is active. Cannot press button.\n");
        return;
    }

    switch (num_servo) {
        case 1:
            servo_left.write(SERVO_PRESS_ANGLE);
            delay(SERVO_PRESS_DURATION);
            servo_left.write(SERVO_RELEASE_ANGLE);
            break;
        case 2:
            servo_middle.write(SERVO_PRESS_ANGLE);
            delay(SERVO_PRESS_DURATION);
            servo_middle.write(SERVO_RELEASE_ANGLE);
            break;
        case 3:
            servo_right.write(SERVO_PRESS_ANGLE);
            delay(SERVO_PRESS_DURATION);
            servo_right.write(SERVO_RELEASE_ANGLE);
            break;
        default:
            Serial.println("Invalid servo number.\n");
            return;
    }
    delay(500);
    
    Serial.println("Button " + String(num_servo) + " pressed.\n");
}

void MotorController::pressSpecificButton(int button) {
    refreshIdle();

    float coord;

    Serial.println("Pressing specific button: " + String(button));

    if (!is_calibrated) {
        Serial.println("Tool not calibrated. Please calibrate first.\n");
        return;
    }

    if (is_emergency_stop) {
        Serial.println("Emergency stop is active. Cannot press button.\n");
        return;
    }

    if (button < 0 || button > 11) {
        Serial.println("Invalid button number. Please press a button between 0 and 11.\n");
        return;
    }

    switch (button) {
        case 0:
            coord = getLineCoordinate(4);
            if (isnan(coord)) { Serial.println("Error: Line 4 coordinate not set. Cannot press button 0 (Clear).\n"); return; }
            moveTo(coord);
            pressButton(2);
            break;
        case 1:
            coord = getLineCoordinate(1);
            if (isnan(coord)) { Serial.println("Error: Line 1 coordinate not set. Cannot press button 1.\n"); return; }
            moveTo(coord);
            pressButton(1);
            break;
        case 2:
            coord = getLineCoordinate(1);
            if (isnan(coord)) { Serial.println("Error: Line 1 coordinate not set. Cannot press button 2.\n"); return; }
            moveTo(coord);
            pressButton(2);
            break;
        case 3:
            coord = getLineCoordinate(1);
            if (isnan(coord)) { Serial.println("Error: Line 1 coordinate not set. Cannot press button 3.\n"); return; }
            moveTo(coord);
            pressButton(3);
            break;
        case 4:
            coord = getLineCoordinate(2);
            if (isnan(coord)) { Serial.println("Error: Line 2 coordinate not set. Cannot press button 4.\n"); return; }
            moveTo(coord);
            pressButton(1);
            break;
        case 5:
            coord = getLineCoordinate(2);
            if (isnan(coord)) { Serial.println("Error: Line 2 coordinate not set. Cannot press button 5.\n"); return; }
            moveTo(coord);
            pressButton(2);
            break;
        case 6:
            coord = getLineCoordinate(2);
            if (isnan(coord)) { Serial.println("Error: Line 2 coordinate not set. Cannot press button 6.\n"); return; }
            moveTo(coord);
            pressButton(3);
            break;
        case 7:
            coord = getLineCoordinate(3);
            if (isnan(coord)) { Serial.println("Error: Line 3 coordinate not set. Cannot press button 7.\n"); return; }
            moveTo(coord);
            pressButton(1);
            break;
        case 8:
            coord = getLineCoordinate(3);
            if (isnan(coord)) { Serial.println("Error: Line 3 coordinate not set. Cannot press button 8.\n"); return; }
            moveTo(coord);
            pressButton(2);
            break;
        case 9:
            coord = getLineCoordinate(3);
            if (isnan(coord)) { Serial.println("Error: Line 3 coordinate not set. Cannot press button 9.\n"); return; }
            moveTo(coord);
            pressButton(3);
            break;
        case 10: // Backspace
            coord = getLineCoordinate(4);
            if (isnan(coord)) { Serial.println("Error: Line 4 coordinate not set. Cannot press button 10.\n"); return; }
            moveTo(coord);
            pressButton(1);
            break;
        case 11: // Submit
            coord = getLineCoordinate(4);
            if (isnan(coord)) { Serial.println("Error: Line 4 coordinate not set. Cannot press button 11.\n"); return; }
            moveTo(coord);
            pressButton(3);
            break;
        default:
            Serial.println("Invalid button number. Please press a button between 0 and 11.\n");
            return;
    }

    Serial.println("Specific button: " + String(button) + " pressed.\n");
}


//* IDLE FUNCTIONS
void MotorController::setIdleTimeout(unsigned long ms) {
    idleTimeoutMs = ms;
    Serial.println("Idle timeout set to: " + String(ms) + " ms\n");
}

void MotorController::checkIdle() {
    if (idleTimeoutMs == 0) return; // disabled timer if 0
    if (!is_disabled && (millis() - lastActivityTimeMs >= idleTimeoutMs)) {
        Serial.println("Motor idle timeout reached — disabling motor.\n");
        disableMotor(); // uses existing function
    }
}

void MotorController::refreshIdle() {
    lastActivityTimeMs = millis();
    if (is_disabled) {
        digitalWrite(ENABLE_PIN, LOW);
        is_disabled = false;
        Serial.println("Motor re-enabled due to activity.\n");
    }
}

//* Saving Line Coordinate
void MotorController::saveLineCoordinate(int line) {
    if (line < 1 || line > 4) {
        Serial.println("Invalid line number. Please provide a line between 1 and 4.\n");
        return;
    }
    float currentPos = getCurrentPosition();
    Serial.println("Saving current position for line: " + String(line));
    setLineCoordinate(line, currentPos);
    Serial.println("Saved current position " + String(currentPos) + " mm as coordinate for line " + String(line) + ".\n");
}

//! Emergency stop and resume functions -- ADD THIS LATER PLEASE!!!