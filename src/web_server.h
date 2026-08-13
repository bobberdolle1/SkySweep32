#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "config.h"

#ifdef MODULE_WEB_SERVER

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>


class SkySweepWebServer {
private:
    AsyncWebServer* httpServer;
    AsyncWebSocket* webSocket;
    bool isRunning;
    uint32_t lastBroadcastTime;
    char managementPassword[64];

    bool requireManagementAuth(AsyncWebServerRequest* request) const;
    bool configureNetwork();
    
    // Route handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleAPI(AsyncWebServerRequest* request);
    void handleConfig(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    
    // WebSocket handler
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);
    
    // HTML generators
    String generateDashboardHTML();

public:
    SkySweepWebServer();
    ~SkySweepWebServer();
    
    bool begin();
    void stop();
    void update();
    
    // Data broadcast to all WebSocket clients
    void broadcastRFData(const char* moduleName, int rssi, bool active);
    void broadcastDroneDetection(const char* droneID, double lat, double lon, float alt);
    void broadcastActivityLevel(const char* level, const char* source);
    void broadcastSystemStatus(const char* status);
    
    IPAddress getIPAddress() const;
    uint16_t getConnectedClients() const;
};

#endif // MODULE_WEB_SERVER
#endif // WEB_SERVER_H
