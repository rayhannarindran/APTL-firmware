#include "mqttManager.h"

#include "../wifiManager/wifiManager.h"
#include "../fsManager/fsManager.h"
#include "../motorController/motorController.h"
#include "../deviceConfig/deviceConfig.h"

extern FSManager fsManager;
extern WifiManager wifiManager;
extern MotorController motorController;

MqttManager* MqttManager::_instance = nullptr;

MqttManager::MqttManager() {
    _instance = this;
}

void MqttManager::init(const IPAddress& broker, uint16_t port, const String& clientId, const char* user, const char* pass) {
    _broker = broker;
    _port = port;
    _clientId = clientId.length() ? clientId : "aptl-device";
    _user = user;
    _pass = pass;
    _client.setServer(_broker, _port);
    _client.setCallback(MqttManager::_internalCallback);
    _client.setSocketTimeout(5);
    _client.setKeepAlive(60);
}

void MqttManager::connect() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (_client.connected()) return;

    Serial.printf("[mqtt] Connecting to MQTT %s:%u ..\n", _broker.toString().c_str(), _port);
    unsigned long start = millis();
    while (!_client.connected()) {
        bool ok;
        if (_user && strlen(_user)) ok = _client.connect(_clientId.c_str(), _user, _pass);
        else ok = _client.connect(_clientId.c_str());
        
        if (ok) {
            Serial.println("[mqtt] connected succesfully.");
            _client.subscribe(_instance->TOPIC_RESP);
            _client.subscribe(_instance->TOPIC_PUSH);
            _instance->requestShared();
            break;
        } else {
            Serial.printf("[mqtt] connect failed, rc=%d. retrying in 5s\n", _client.state());
            delay(5000);
        }
        // optional safety timeout (avoid infinite block on WiFi lost)
        if (millis() - start > 30000) break;
    }
}

void MqttManager::loop() {
    if (_client.connected()) {
        _client.loop();
    }
}

void MqttManager::requestShared() {
  const char* keys = "kodetoken,up,down,press1,press2,press3,stop,setmax,row1,row2,row3,row4,newssid,newpass";
  char payload[128];
  snprintf(payload, sizeof(payload), "{\"sharedKeys\":\"%s\"}", keys);
  _client.publish(_instance->TOPIC_REQ, payload);
}

void MqttManager::applyShared(JsonVariant root) {
    // incoming payload can be either { "shared": { ... } } or just { ... }
    JsonVariant shared = root["shared"];
    JsonVariant obj    = shared.isNull() ? root : shared;

    bool changed = false;

    if (obj["kodetoken"].is<String>()) {
    String nv = obj["kodetoken"].as<String>();
    if (nv != kodetoken) { kodetoken = nv; changed = true; }
    }

    if (obj["up"].is<int>()) {
        int nv = obj["up"].as<int>();
        if (nv != up) { up = nv; changed = true; }
    }

    if (obj["down"].is<int>()) {
        int nv = obj["down"].as<int>();
        if (nv != down) { down = nv; changed = true; }
    }

    if (obj["press1"].is<int>()) {
        int nv = obj["press1"].as<int>();
        if (nv != press1) { press1 = nv; changed = true; }
    }
  
    if (obj["press2"].is<int>()) {
        int nv = obj["press2"].as<int>();
        if (nv != press2) { press2 = nv; changed = true; }
    }

    if (obj["press3"].is<int>()) {
        int nv = obj["press3"].as<int>();
        if (nv != press3) { press3 = nv; changed = true; }
    }

    if (obj["stop"].is<int>()) {
        int nv = obj["stop"].as<int>();
        if (nv != stop) { stop = nv; changed = true; }
    }

    if (obj["setmax"].is<int>()) {
        int nv = obj["setmax"].as<int>();
        if (nv != setmax) { setmax = nv; changed = true; }
    }

    if (obj["row1"].is<int>()) {
        int nv = obj["row1"].as<int>();
        if (nv != row1) { row1 = nv; changed = true; }
    }

    if (obj["row2"].is<int>()) {
        int nv = obj["row2"].as<int>();
        if (nv != row2) { row2 = nv; changed = true; }
    }

    if (obj["row3"].is<int>()) {
        int nv = obj["row3"].as<int>();
        if (nv != row3) { row3 = nv; changed = true; }
    }

    if (obj["row4"].is<int>()) {
        int nv = obj["row4"].as<int>();
        if (nv != row4) { row4 = nv; changed = true; }
    }

    if (obj["newssid"].is<String>()) {
        String nv = obj["newssid"].as<String>();
        if (nv != newssid) { newssid = nv; changed = true; }
    }

    if (obj["newpass"].is<String>()) {
        String nv = obj["newpass"].as<String>();
        if (nv != newpass) { newpass = nv; changed = true; }
    }

    if (changed) subUpdated = true;
}

void MqttManager::publishTelemetry() {
  // Siapkan JSON telemetry
  posisi = motorController.getCurrentPosition();

  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"posisi\":%.2f,\"statusaptl\":%d}",
           posisi, statusaptl);

  bool ok = _client.publish(_instance->TOPIC_PUB, payload);
  Serial.printf("[pub] %s | %s\n", ok ? "OK" : "FAIL", payload);
}

void MqttManager::publishStatus(int status) {
  statusaptl = status;
  publishTelemetry();
}

void MqttManager::printSubTick() {
  Serial.printf("[sub] updated=%s | kodetoken=%s | up=%d | down=%d | press1=%d | press2=%d | press3=%d | stop=%d | setmax=%d | row1=%d | row2=%d | row3=%d | row4=%d | newssid=%s | newpass=%s\n",
                subUpdated ? "Yes" : "No", kodetoken.c_str(), up, down, press1, press2, press3, stop, setmax, row1, row2, row3, row4, newssid.c_str(), newpass.c_str());
  subUpdated = false;
}

void MqttManager::_internalCallback(char* topic, byte* payload, unsigned int length) {
    if (!_instance) return;

    static char payloadBuf[768];
    unsigned int n = (length < sizeof(payloadBuf) - 1) ? length : sizeof(payloadBuf) - 1;
    memcpy(payloadBuf, payload, n);
    payloadBuf[n] = '\0';

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payloadBuf);
    if (err) {
        Serial.printf("MQTT JSON parse error: %s\n", err.c_str());
        return;
    }

    String t(topic ? topic : "");
    if (t.startsWith("v1/devices/me/attributes/response/") || t == String(_instance->TOPIC_PUSH)) {
        _instance->applyShared(doc.as<JsonVariant>());
    }
}

void MqttManager::processCommands() {
    if (!subUpdated) return;

    String tempKodeToken = kodetoken;
    int tempUp = up;
    int tempDown = down;
    int tempPress1 = press1;
    int tempPress2 = press2;
    int tempPress3 = press3;
    int tempStop = stop;
    int tempSetMax = setmax;
    int tempRow1 = row1;
    int tempRow2 = row2;
    int tempRow3 = row3;
    int tempRow4 = row4;
    String tempNewSsid = newssid;
    String tempNewPass = newpass;

    subUpdated = false;
    bool needSave = false;

    //* WiFi Settings
    if (tempNewSsid.length() && tempNewSsid != prev_newssid && tempNewSsid != getWiFiSSID()) {
        publishStatus(51); //* Setting WiFi SSID/Password

        Serial.printf("[mqtt] newssid received: %s\n", tempNewSsid.c_str());
        setWiFiCredentials(tempNewSsid, tempNewPass);
        fsManager.saveConfig();
        wifiManager.init(getWiFiSSID(), getWiFiPassword());
        wifiManager.connect();

        publishStatus(0); //* Idle

        prev_newssid = tempNewSsid;
        prev_newpass = tempNewPass;
    }

    //* Motor Commands
    if (tempUp && prev_up == 0) {
        publishStatus(11); //* Moving Up

        Serial.println("[mqtt] Command: UP");
        motorController.moveBy(-10);

        publishStatus(0); //* Idle
    }
    prev_up = tempUp;

    if (tempDown && prev_down == 0) {
        publishStatus(12); //* Moving Down

        Serial.println("[mqtt] Command: DOWN");
        motorController.moveBy(10);

        publishStatus(0); //* Idle
    }
    prev_down = tempDown;

    if (tempPress1 && prev_press1 == 0) {
        publishStatus(21); //* Pressing Button 1

        Serial.println("[mqtt] Command: PRESS 1");
        motorController.pressButton(1);

        publishStatus(0); //* Idle
    }
    prev_press1 = tempPress1;

    if (tempPress2 && prev_press2 == 0) {
        publishStatus(22); //* Pressing Button 2

        Serial.println("[mqtt] Command: PRESS 2");
        motorController.pressButton(2);

        publishStatus(0); //* Idle
    }
    prev_press2 = tempPress2;

    if (tempPress3 && prev_press3 == 0) {
        publishStatus(23); //* Pressing Button 3

        Serial.println("[mqtt] Command: PRESS 3");
        motorController.pressButton(3); 

        publishStatus(0); //* Idle
    }
    prev_press3 = tempPress3;

    if (tempSetMax && prev_setmax == 0) {
        publishStatus(41); //* Setting Max Position

        Serial.println("[mqtt] Command: SET MAX POSITION");
        setMaxPosition(motorController.getCurrentPosition());
        motorController.setMaximumPosition(motorController.getCurrentPosition());

        publishStatus(0); //* Idle
    }
    prev_setmax = tempSetMax;

    //* Saving Line Coordinates
    // set rows when changed (replace functionality)
    if (tempRow1 != prev_row1) { 
        publishStatus(31); //* Setting Row 1 Position

        setLineCoordinate(1, motorController.getCurrentPosition()); 
        needSave = true; 
    }
    prev_row1 = tempRow1;

    if (tempRow2 != prev_row2) { 
        publishStatus(32); //* Setting Row 2 Position

        setLineCoordinate(2, motorController.getCurrentPosition()); 
        needSave = true; 
    }
    prev_row2 = tempRow2;

    if (tempRow3 != prev_row3) { 
        publishStatus(33); //* Setting Row 3 Position

        setLineCoordinate(3, motorController.getCurrentPosition()); 
        needSave = true; 
    }
    prev_row3 = tempRow3;
    
    if (tempRow4 != prev_row4) { 
        publishStatus(34); //* Setting Row 4 Position

        setLineCoordinate(4, motorController.getCurrentPosition()); 
        needSave = true; 
    }
    prev_row4 = tempRow4;

    if (needSave) {
        fsManager.saveConfig();
        Serial.println("[mqtt] Line coordinates updated and saved.");
        publishStatus(0); //* Idle
    }

    //* Token Input
    if (tempKodeToken.length() && tempKodeToken != prev_kodetoken) {
        publishStatus(1); //* Isi Token

        Serial.printf("[mqtt] Kode Token received: %s\n", tempKodeToken.c_str());

        for (size_t i = 0; i < tempKodeToken.length(); i++) {
            char c = tempKodeToken[i];
            if (isdigit(static_cast<unsigned char>(c))) {
                int digit = c - '0';
                if (digit >= 0 && digit <= 11){
                    Serial.printf("[mqtt] Pressing button %d\n", digit);
                    motorController.pressSpecificButton(digit);
                    delay(250); // delay between button presses
                } else {
                    Serial.printf("[mqtt] Invalid button number: %d\n", digit);
                }
            }
        }
        motorController.moveTo(0); // return to home after input

        publishStatus(0); //* Idle

        prev_kodetoken = tempKodeToken;
    }
}

bool MqttManager::is_connected() {
    return _client.connected();
}