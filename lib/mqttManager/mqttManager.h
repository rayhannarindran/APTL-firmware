#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

class WifiManager;
class FSManager;
class MotorController;

class MqttManager {
public:
    MqttManager();
    void init(const IPAddress& broker, uint16_t port = 1883,
              const String& clientId = "aptl-client", const char* user = "", const char* pass = nullptr);
    void connect();
    void loop();
    void processCommands();

    void requestShared();
    void applyShared(JsonVariant root);

    void publishTelemetry();
    void printSubTick();

    bool is_connected();

private:
    WiFiClient      _wifiClient;
    PubSubClient    _client{_wifiClient};
    IPAddress       _broker;
    uint16_t        _port{1883};
    String          _clientId;
    const char*     _user;
    const char*     _pass;

    // Topic standar ThingsBoard
    const char* TOPIC_PUB  = "v1/devices/me/telemetry";             // publish telemetry
    const char* TOPIC_REQ  = "v1/devices/me/attributes/request/1";  // request shared attrs
    const char* TOPIC_RESP = "v1/devices/me/attributes/response/+"; // response attrs
    const char* TOPIC_PUSH = "v1/devices/me/attributes";            // push realtime

    // Telemetry yang akan dikirim (random)
    float posisi = 0.0f;
    int   statusaptl = 0;

    // Cache shared attributes yang diterima
    String kodetoken = "";
    int   home = 0;
    int   up = 0, down = 0;
    int   press1 = 0, press2 = 0, press3 = 0;
    int   stop = 0;
    int   setmax = 0;
    int   row1 = 0, row2 = 0, row3 = 0, row4 = 0;
    String newssid = "", newpass = "";

    // Previous state to detect changes
    String prev_kodetoken = "default";
    int prev_home = 0;
    int prev_up = 0, prev_down = 0;
    int prev_press1 = 0, prev_press2 = 0, prev_press3 = 0;
    int prev_stop = 0;
    int prev_setmax = 0;
    int prev_row1 = 0, prev_row2 = 0, prev_row3 = 0, prev_row4 = 0;
    String prev_newssid = "default", prev_newpass = "default";

    volatile bool subUpdated = false;

    static MqttManager* _instance;
    static void _internalCallback(char* topic, byte* payload, unsigned int length);

    void (*_userCb)(const String& topic, const String& payload) = nullptr;
};

#endif // MQTT_MANAGER_H