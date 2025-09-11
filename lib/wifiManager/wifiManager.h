#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

#define AP_SSID "APTL"

class WifiManager {
public:
    WifiManager();
    void init(const String& ssid, const String& password);
    void connect();
    void disconnect();

    // Setters
    void setCredentials(const String& new_ssid, const String& new_password);

    // Getters
    bool getConnectionStatus() const;
    String getSSID() const;
    String getPassword() const;
    String getIPAddress() const;
    String getMACAddress() const;
    
    // Web server handling
    void apMode();

private:
    String ssid;
    String password;
    bool isConnected;
};

#endif