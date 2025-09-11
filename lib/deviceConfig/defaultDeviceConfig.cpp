//! RENAME THIS FILE TO deviceConfig.cpp AND EDIT THE SETTINGS BELOW

#include "deviceConfig.h"

//* ================== MQTT (ThingsBoard - Non TLS) ==================
static const IPAddress MQTT_IP(0, 0, 0, 0);
static const uint16_t  MQTT_PORT = 1883;
static const char*     TOKEN      = "";

//* DEVICE CONFIGURATION
static String deviceName = "";                      //* Set using AP-mode from web-config / changed manually later
static String deviceID = "";                        //* Set in setup() after WiFi init

static String wifiSSID = "";                        //* Set using AP-mode from web-config / changed manually later
static String wifiPassword = "";                    //* Set using AP-mode from web-config / changed manually later

static float maxPosition = 0;                       //* Max position of the actuator in mm
static std::map<int, float> lineCoordinates = {     //* Set when saving config from web-config
    {1, 0.0},
    {2, 0.0},
    {3, 0.0},
    {4, 0.0},
};

//* FUNCTIONS
void printConfig() {
    Serial.println("===== Device Configuration =====");
    Serial.print("Device Name: ");
    Serial.println(deviceName);
    Serial.print("Device ID: ");
    Serial.println(deviceID);
    Serial.print("WiFi SSID: ");
    Serial.println(wifiSSID);
    Serial.print("WiFi Password: ");
    Serial.println("********");
    Serial.println("Line Coordinates (mm):");
    for (const auto& pair : lineCoordinates) {
        Serial.print("  Line ");
        Serial.print(pair.first);
        Serial.print(": ");
        Serial.print(pair.second);
        Serial.println(" mm");
    }
    Serial.println("================================");
}

const IPAddress& getMqttIP() { return MQTT_IP; }
uint16_t getMqttPort() { return MQTT_PORT; }
const char* getMqttToken() { return TOKEN; }

void setDeviceName(const String& name) { deviceName = name; }
void setDeviceID(const String& id) { deviceID = id; }
void setWiFiCredentials(const String& ssid, const String& password) {
    wifiSSID = ssid;
    wifiPassword = password;
}

void setMaxPosition(float mm) {
    if (mm > 0) { maxPosition = mm; }
}
void setLineCoordinates(const std::map<int,float>& coords) { lineCoordinates = coords; }
void setLineCoordinate(int line, float coordinate) {
    if (line <= 0 || line > 4) return;
    if (coordinate < 0.0f) coordinate = 0.0f;
    lineCoordinates[line] = coordinate;
}

String getDeviceName() { return deviceName; }
String getDeviceID() { return deviceID; }
String getWiFiSSID() { return wifiSSID; }
String getWiFiPassword() { return wifiPassword; }

float getMaxPosition() { return maxPosition; }
const std::map<int, float>& getLineCoordinates() { return lineCoordinates; }
float getLineCoordinate(int line) {
    auto it = lineCoordinates.find(line);
    return (it == lineCoordinates.end()) ? NAN : it->second;
}