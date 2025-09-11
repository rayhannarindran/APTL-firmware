#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "FS.h"
#include "deviceConfig.h"

#define FORMAT_LITTLEFS_IF_FAILED true

class FSManager {
public:
    void init();
    void saveConfig();
    void loadConfig();
    void readConfig();
    void formatFS();
};

#endif // FS_MANAGER_H