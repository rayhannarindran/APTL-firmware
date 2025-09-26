#include "LittleFS.h"

void setup() {
    Serial.begin(115200);
    if (!LittleFS.begin(true)) {  // `true` = auto format
        Serial.println("LittleFS Mount Failed, formatting...");
    } else {
        Serial.println("LittleFS mounted successfully.");
    }
}

void loop() {}