#include <Arduino.h>
#include "deviceConfig.h"
#include "fsManager.h"

FSManager fsManager;

void setup(){
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\nFormatting APTL File System...");
    fsManager.init();
    fsManager.formatFS();
}

void loop(){
}