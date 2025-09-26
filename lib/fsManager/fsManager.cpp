#include "fsManager.h"

void FSManager::init() {
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        Serial.println("Failed to mount or format LittleFS. Please check the filesystem.");
        return;
    }
    Serial.println("LittleFS initialized successfully.");

    if(!LittleFS.exists("/config.json")) {
        Serial.println("Config file does not exist. Creating default config.");
        saveConfig();
    } else {
        Serial.println("Config file found. Loading config.");
        loadConfig();
    }
}

void FSManager::saveConfig() {
    JsonDocument doc;

    doc["deviceName"] = getDeviceName();
    doc["deviceID"] = getDeviceID();
    doc["wifiSSID"] = getWiFiSSID();
    doc["wifiPassword"] = getWiFiPassword();
    doc["maxPosition"] = getMaxPosition();
    JsonObject lineCoords = doc["lineCoordinates"].to<JsonObject>();
    for (const auto& pair : getLineCoordinates()) {
        lineCoords[String(pair.first)] = pair.second;
    }

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to open config file for writing.");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write config to file.\n");
    } else {
        Serial.println("Config saved successfully.\n");
    }

    file.close();
}

void FSManager::loadConfig() {
    if (!LittleFS.exists("/config.json")) {
        Serial.println("Config file does not exist. Please save/create the config first.");
        return;
    }

    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Failed to open config file for reading.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to parse config file.");
        file.close();
        return;
    }

    setDeviceName(doc["deviceName"].as<String>());
    setDeviceID(doc["deviceID"].as<String>());
    setWiFiCredentials(doc["wifiSSID"].as<String>(), doc["wifiPassword"].as<String>());
    setMaxPosition(doc["maxPosition"].as<float>());

    std::map<int, float> newCoords;
    if (doc["lineCoordinates"].is<JsonObject>()) {
        JsonObject obj = doc["lineCoordinates"].as<JsonObject>();
        for (JsonPair pair : obj) {
            int key = atoi(pair.key().c_str());
            newCoords[key] = pair.value().as<float>();
        }
    }
    setLineCoordinates(newCoords);

    Serial.println("Config loaded successfully.\n");
    file.close();
}

void FSManager::readConfig() {
    if (!LittleFS.exists("/config.json")) {
        Serial.println("Config file does not exist.");
        return;
    }

    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Failed to open config file for reading.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (!error) {
        Serial.println("Current Config:");
        serializeJsonPretty(doc, Serial);
        Serial.println("\n");
    } else {
        Serial.println("Failed to parse config file.\n");
    }
    file.close();
}

void FSManager::formatFS() {
    if (LittleFS.format()) {
        Serial.println("LittleFS formatted successfully.\n");
    } else {
        Serial.println("Failed to format LittleFS.\n");
    }
}