#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

#define WEB_SERVER_PORT 80
#define WEBSOCKET_PORT 81
#define AP_SSID "SkySweep32_AP"
#define AP_PASSWORD "skysweep2026"

struct WebServerConfig {
    char ssid[32];
    char password[64];
    IPAddress localIP;
    IPAddress gateway;
    IPAddress subnet;
    bool apMode;
};

class SkySweepWebServer {
private:
    AsyncWebServer* httpServer;
    AsyncWebSocket* webSocket;
    WebServerConfig config;
    bool isRunning;
    
    void handleRoot(AsyncWebServerRequest* request);
    void handleAPI(AsyncWebServerRequest* request);
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                              AwsEventType type, void* arg, uint8_t* data, size_t len);
    
    String generateDashboardHTML();
    String generateConfigHTML();

public:
    SkySweepWebServer();
    
    bool begin(bool accessPointMode = true);
    void stop();
    void update();
    
    void broadcastRFData(const char* moduleName, int rssi, bool active);
    void broadcastDroneDetection(const char* droneID, double lat, double lon, float alt);
    void broadcastSystemStatus(const char* status);
    
    void setConfig(const WebServerConfig& newConfig);
    WebServerConfig getConfig() const;
    
    IPAddress getIPAddress() const;
    uint16_t getConnectedClients() const;
};

#endif
