#include <Arduino.h>
#include "deviceConfig.h"
#include "fsManager.h"
#include "wifiManager.h"
#include "mqttManager.h"
#include "motorController.h"

FSManager fsManager;
WifiManager wifiManager;
MqttManager mqttManager;
MotorController motorController;

// Backend connection checks
static unsigned long lastWifiAttempt = 0;
static uint8_t wifiFailedReconnects = 0;
static const uint8_t MAX_WIFI_FAILED_RECONNECTS = 5;
const unsigned long WIFI_RECONNECT_INTERVAL = 5000;

static unsigned long lastMqttAttempt = 0;
static unsigned long lastMqttSubReq = 0;
static unsigned long lastMqttSubLog = 0;
static unsigned long lastMqttPub    = 0;

const unsigned long MQTT_RECONNECT_INTERVAL = 3000;
const unsigned long MQTT_PUB_INTERVAL       = 1000;  // kirim telemetry tiap 1s
const unsigned long MQTT_SUB_POLL_INTERVAL  = 5000;  // request shared attrs tiap 5s (selain push)
const unsigned long MQTT_SUB_LOG_INTERVAL   = 2000;  // log status SUB tiap 2s


//* Main Program
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\nStarting APTL firmware...\n");

    //* Setting DeviceID
    setDeviceID(WiFi.macAddress());
    Serial.println("Device ID (MAC): " + getDeviceID() + "\n");

    //* Initializing File System
    fsManager.init();

    //* Initializing WiFi
    wifiManager.init(getWiFiSSID(), getWiFiPassword());
    wifiManager.connect();
    if (!wifiManager.getConnectionStatus()) {
        Serial.println("WiFi not connected. Starting AP mode...\n");
        wifiManager.apMode();
        fsManager.saveConfig();
    } else {
        mqttManager.init(getMqttIP(), getMqttPort(), getDeviceID(), getMqttToken(), nullptr);
        mqttManager.connect();
    }

    //* Initializing Motor Controller
    motorController.setup();
    motorController.calibrate();


    // //! TESTS
    // //* Config Check
    // fsManager.readConfig();

    //* Motor Test
    motorController.setMaxPositionSetting(true);
    motorController.setMaximumPosition(120);
    motorController.setMaxPositionSetting(false);

    // //* Servo Test
    // motorController.pressButton(1);
    // motorController.moveTo(20);
    // motorController.pressButton(2);
    // motorController.moveTo(80);
    // motorController.pressButton(3);
    // motorController.moveTo(110);

    // //* Specific Button Test
    // motorController.pressSpecificButton(1);
    // motorController.pressSpecificButton(2);
    // motorController.pressSpecificButton(3);
    // motorController.pressSpecificButton(4);
    // motorController.pressSpecificButton(5);
    // motorController.pressSpecificButton(6);
    // motorController.pressSpecificButton(7);
    // motorController.pressSpecificButton(8);
    // motorController.pressSpecificButton(9);
    // motorController.pressSpecificButton(0);
    // motorController.pressSpecificButton(10); // Backspace
    // motorController.pressSpecificButton(11); // Submit
    // motorController.moveTo(0);
}

void loop() {
    motorController.checkIdle();
    mqttManager.loop();
    mqttManager.processCommands();

    unsigned long now = millis();

    if (!wifiManager.getConnectionStatus()) {
        if (now - lastWifiAttempt >= WIFI_RECONNECT_INTERVAL) {
            lastWifiAttempt = now;
            Serial.println("WiFi disconnected. Attempting to reconnect...");
            wifiManager.connect();

            if (!wifiManager.getConnectionStatus()) {
                wifiFailedReconnects++;
                Serial.printf("WiFi reconnect failed (%u/%u)\n", wifiFailedReconnects, MAX_WIFI_FAILED_RECONNECTS);
                if (wifiFailedReconnects >= MAX_WIFI_FAILED_RECONNECTS) {
                    Serial.println("Max WiFi failed reconnects reached — switching to AP mode.");
                    wifiManager.apMode();
                    fsManager.saveConfig();
                }
            } else {
                wifiFailedReconnects = 0;
            }
        }
    } else {
        // WiFi is connected — reset counters and handle MQTT reconnects
        if (wifiFailedReconnects != 0) wifiFailedReconnects = 0;

        if (!mqttManager.is_connected()) {
            if (now - lastMqttAttempt >= MQTT_RECONNECT_INTERVAL) {
                lastMqttAttempt = now;
                Serial.println("MQTT disconnected. Attempting to reconnect...");
                mqttManager.connect();
            }
        }
    }

    if (mqttManager.is_connected()) {
        if (now - lastMqttSubReq >= MQTT_SUB_POLL_INTERVAL) {
            lastMqttSubReq = now;
            mqttManager.requestShared();
        }

        if (now - lastMqttSubLog >= MQTT_SUB_LOG_INTERVAL) {
            lastMqttSubLog = now;
            mqttManager.printSubTick();
        }

        if (now - lastMqttPub >= MQTT_PUB_INTERVAL) {
            lastMqttPub = now;
            mqttManager.publishTelemetry();
        }
    }
    
    delay(50);
}
