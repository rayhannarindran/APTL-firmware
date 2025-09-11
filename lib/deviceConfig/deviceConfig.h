#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>
#include <cmath>
#include <map>

// Functions
void printConfig();

const IPAddress&    getMqttIP();
uint16_t            getMqttPort();
const char*         getMqttToken();

void setDeviceName(const String& name);
void setDeviceID(const String& id);
void setWiFiCredentials(const String& ssid, const String& password);

void setMaxPosition(float mm);
void setLineCoordinate(int index, float mm);
void setLineCoordinates(const std::map<int,float>& coords);

String getDeviceName();
String getDeviceID();
String getWiFiSSID();
String getWiFiPassword();

float getMaxPosition();
const std::map<int, float>& getLineCoordinates();
float getLineCoordinate(int index);

#endif // DEVICE_CONFIG_H