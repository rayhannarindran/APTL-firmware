#include "wifiManager.h"

#include "../fsManager/fsManager.h"
extern FSManager fsManager;

WifiManager::WifiManager() {}

void WifiManager::init(const String& ssid, const String& password) {
    this->ssid = ssid;
    this->password = password;
}

void WifiManager::connect() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Successfully connected to WiFi.");
        Serial.println("WiFi SSID: " + ssid);
        Serial.println("WiFi IP Address: " + WiFi.localIP().toString() + "\n");
    } else {
        Serial.println("Failed connecting to WiFi.\n");
    }
}

void WifiManager::disconnect() {
    Serial.println("Disconnecting from WiFi...");
    WiFi.disconnect();
    Serial.println("Disconnected from WiFi.\n");
}

//* Setters
void WifiManager::setCredentials(const String& new_ssid, const String& new_password) {
    ssid = new_ssid;
    password = new_password;
}

//* Getters
bool WifiManager::getConnectionStatus() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getSSID() const {
    return ssid;
}

String WifiManager::getPassword() const {
    return password;
}

String WifiManager::getIPAddress() const {
    return WiFi.localIP().toString();
}

String WifiManager::getMACAddress() const {
    return WiFi.macAddress();
}

// simple URL-decode helper
static String urlDecode(const String &input) {
    String ret;
    char c;
    for (size_t i = 0; i < input.length(); ++i) {
        c = input[i];
        if (c == '+') ret += ' ';
        else if (c == '%' && i + 2 < input.length()) {
            String hex = input.substring(i + 1, i + 3);
            char decoded = (char) strtol(hex.c_str(), nullptr, 16);
            ret += decoded;
            i += 2;
        } else {
            ret += c;
        }
    }
    return ret;
}

//! AP Mode -- CHANGE LATER BASED ON BACKEND!
void WifiManager::apMode() {
    Serial.println("Starting Access Point mode...");

    IPAddress apIP(192,168,4,1);
    IPAddress netMsk(255,255,255,0);

    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(AP_SSID);
    Serial.println("Access Point started.");

    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP Address: ");
    Serial.println(ip);

    // DNS server to redirect all queries to our AP IP
    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    dnsServer.start(DNS_PORT, "*", apIP);

    // Scan networks for the dropdown
    Serial.println("Scanning for available networks...\n");
    int n = WiFi.scanNetworks();
    Serial.println("Found " + String(n) + " networks!\n");

    String networksHtml;
    networksHtml.reserve(1024);
    networksHtml += "<option value=\"\">-- Select network --</option>\n";
    for (int i = 0; i < n; ++i) {
        String ss = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        String enc = ss; // browser will URL-encode
        networksHtml += "<option value=\"";
        networksHtml += enc;
        networksHtml += "\">";
        networksHtml += ss;
        networksHtml += " (";
        networksHtml += String(rssi);
        networksHtml += " dBm)</option>\n";
    }

    WiFiServer server(80);
    server.begin();
    Serial.println("Provisioning server started on port 80. Connect to AP and captive portal will open.");

    // blocking server loop (intentional: AP mode exclusive)
    while (true) {
        dnsServer.processNextRequest();  // handle DNS

        WiFiClient client = server.available();
        if (!client) {
            delay(10); // yield to RTOS / WDT
            continue;
        }

        // read the request (header)
        String req = client.readStringUntil('\r');
        client.read(); // consume '\n'

        // parse resource path
        String resource;
        int firstSpace = req.indexOf(' ');
        if (firstSpace >= 0) {
            int secondSpace = req.indexOf(' ', firstSpace + 1);
            if (secondSpace > firstSpace) {
                resource = req.substring(firstSpace + 1, secondSpace);
            }
        }

        // read & discard rest of headers
        while (client.available()) {
            String line = client.readStringUntil('\r');
            client.read();
            if (line.length() <= 1) break;
        }

        // --- handle save request ---
        if (resource.startsWith("/save")) {
            int q = resource.indexOf('?');
            String query = (q >= 0) ? resource.substring(q + 1) : String();
            String newSsid, newPass;

            int idx = 0;
            while (idx < (int)query.length()) {
                int amp = query.indexOf('&', idx);
                String pair = (amp >= 0) ? query.substring(idx, amp) : query.substring(idx);
                int eq = pair.indexOf('=');
                if (eq > 0) {
                    String key = pair.substring(0, eq);
                    String val = pair.substring(eq + 1);
                    if (key == "ssid") newSsid = urlDecode(val);
                    else if (key == "pass") newPass = urlDecode(val);
                }
                if (amp < 0) break;
                idx = amp + 1;
            }

            if (newSsid.length()) {
                setCredentials(newSsid, newPass);
                setWiFiCredentials(newSsid, newPass);
                fsManager.saveConfig();
                Serial.println("Saved new WiFi credentials:");
                Serial.println("SSID: " + newSsid);
                Serial.print("Password: ");
                Serial.println(newPass.length() ? "********" : "(empty)");
            } else {
                Serial.println("No SSID provided; ignoring save request.");
            }

            String body = "<html><body><h2>Saved. Device will restart...</h2></body></html>";
            client.printf("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\nConnection: close\r\n\r\n%s",
                          (unsigned)body.length(), body.c_str());
            client.stop();
            delay(250);
            ESP.restart();
            return;
        }

        // --- serve config page ---
        String body;
        body.reserve(4096);
        body  = "<!doctype html><html><head><meta charset='utf-8'><title>APTL Provision</title>";
        body += "<style>";
        body += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background: #f2f2f2; }";
        body += "h2 { font-size: 56px; margin-bottom: 40px; }";
        body += "form { display: inline-block; text-align: left; background: #fff; padding: 40px 60px; border-radius: 20px; box-shadow: 0 8px 16px rgba(0,0,0,0.2); }";
        body += "label { font-size: 36px; display: block; margin-top: 30px; }";
        body += "input, select { width: 100%; padding: 20px; margin-top: 10px; font-size: 32px; border: 2px solid #ccc; border-radius: 12px; }";
        body += "input[type='submit'] { margin-top: 40px; background: #007BFF; color: white; border: none; cursor: pointer; font-size: 36px; padding: 20px 40px; border-radius: 12px; }";
        body += "input[type='submit']:hover { background: #0056b3; }";
        body += "p { font-size: 24px; margin-top: 30px; }";
        body += "</style></head><body>";

        body += "<h2>Configure WiFi</h2>";
        body += "<form method='GET' action='/save'>";
        body += "<label for='ssid'>Nearby networks:</label>";
        body += "<select id='ssid' name='ssid'>";
        body += networksHtml;
        body += "</select>";
        body += "<label for='pass'>Password:</label>";
        body += "<input type='password' id='pass' name='pass' />";
        body += "<input type='submit' value='Save and Restart' />";
        body += "</form>";
        body += "<p>If your network does not appear, type SSID manually or refresh the page.</p>";
        body += "</body></html>";

        client.printf("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\nConnection: close\r\n\r\n%s",
                      (unsigned)body.length(), body.c_str());
        client.stop();
    }
}