#include "web_server.h"

#ifdef MODULE_WEB_SERVER

#include "config_manager.h"
#include "power_manager.h"
#ifdef MODULE_SD_CARD
#include <SD.h>
#endif

// Host-side static analysis does not provide Arduino's PROGMEM macro.  The
// fallback is a no-op on desktop builds and leaves the ESP32 definition intact.
#ifndef PROGMEM
#define PROGMEM
#endif

// ============================================================================
// Dashboard HTML (stored in PROGMEM to save RAM)
// ============================================================================

#include "generated/dashboard_html_gz.h"

// ============================================================================
// SkySweepWebServer Implementation
// ============================================================================

SkySweepWebServer::SkySweepWebServer()
    : httpServer(nullptr), webSocket(nullptr), isRunning(false), lastBroadcastTime(0) {
    managementPassword[0] = '\0';
}

SkySweepWebServer::~SkySweepWebServer() {
    stop();
}

bool SkySweepWebServer::requireManagementAuth(AsyncWebServerRequest* request) const {
    if (request->authenticate(WEB_MANAGEMENT_USERNAME, managementPassword)) {
        return true;
    }
    request->requestAuthentication("SkySweep32 management");
    return false;
}

bool SkySweepWebServer::configureNetwork() {
    const RuntimeConfig& runtime = configManager.get();
    strlcpy(managementPassword, runtime.wifiPassword, sizeof(managementPassword));
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(runtime.wifiSSID, runtime.wifiPassword, runtime.wifiChannel, 0,
                     WIFI_MAX_CLIENTS)) {
        Serial.println("[WEB] Failed to start AP");
        return false;
    }
    powerManager.applyWiFiPolicy();
    Serial.printf("[WEB] AP started: %s (IP: %s)\n", runtime.wifiSSID,
                  WiFi.softAPIP().toString().c_str());
    return true;
}

bool SkySweepWebServer::begin() {
    if (!configureNetwork()) return false;
    
    // Create HTTP server and WebSocket
    httpServer = new AsyncWebServer(WEB_SERVER_PORT);
    webSocket = new AsyncWebSocket("/ws");
    
    // WebSocket event handler
    webSocket->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                              AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    
    httpServer->addHandler(webSocket);
    
    // Routes
    httpServer->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleRoot(request);
    });
    
    httpServer->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleAPI(request);
    });
    
    httpServer->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleConfig(request);
    });
    
    #ifdef MODULE_SD_CARD
    // Logs can contain GNSS observations. They are sensitive and require the
    // same per-device management credential as configuration changes.
    httpServer->on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireManagementAuth(request)) return;
        JsonDocument doc;
        JsonArray files = doc.to<JsonArray>();
        if (SD.exists("/logs")) {
            File root = SD.open("/logs");
            if (!root || !root.isDirectory()) {
                request->send(500, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Could not open logs directory\"}");
                return;
            }
            for (File file = root.openNextFile(); file; file = root.openNextFile()) {
                if (!file.isDirectory()) {
                    JsonObject entry = files.add<JsonObject>();
                    entry["name"] = file.name();
                    entry["size"] = file.size();
                }
                file.close();
            }
            root.close();
        }
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    httpServer->on("/api/logs/download", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireManagementAuth(request)) return;
        if (!request->hasParam("file")) {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Missing 'file' param\"}");
            return;
        }
        const String filename = request->getParam("file")->value();
        if (filename.indexOf("..") >= 0 || filename.indexOf('/') >= 0 ||
            filename.indexOf('\\') >= 0 || filename.indexOf('\r') >= 0 ||
            filename.indexOf('\n') >= 0) {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Invalid filename\"}");
            return;
        }
        const String filepath = "/logs/" + filename;
        if (!SD.exists(filepath)) {
            request->send(404, "application/json",
                          "{\"status\":\"error\",\"msg\":\"File not found\"}");
            return;
        }
        AsyncWebServerResponse* response =
            request->beginResponse(SD, filepath, "text/plain");
        response->addHeader("Content-Disposition", "attachment; filename=" + filename);
        request->send(response);
    });
    #endif
    
    httpServer->onNotFound([this](AsyncWebServerRequest* request) {
        this->handleNotFound(request);
    });
    
    // Runtime configuration is persisted for the next boot. The existing
    // network stays active until a deliberate reboot; this API never claims
    // hot application.
    httpServer->on(
        "/api/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
            String* body = static_cast<String*>(request->_tempObject);
            if (!requireManagementAuth(request)) {
                delete body;
                request->_tempObject = nullptr;
                return;
            }
            if (request->contentLength() > WEB_CONFIG_BODY_MAX_BYTES) {
                delete body;
                request->_tempObject = nullptr;
                request->send(413, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Request too large\"}");
                return;
            }
            const bool saved = body && configManager.fromJSON(body->c_str());
            delete body;
            request->_tempObject = nullptr;
            if (!saved) {
                request->send(400, "application/json",
                              "{\"status\":\"error\",\"msg\":\"Invalid configuration\"}");
                return;
            }
            request->send(200, "application/json",
                          "{\"status\":\"ok\",\"msg\":\"Saved; reboot required for network changes\"}");
            Serial.println("[CONFIG] Updated via authenticated web API");
        },
        nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index,
           size_t total) {
            if (index == 0) {
                if (total > WEB_CONFIG_BODY_MAX_BYTES) return;
                String* body = new String();
                body->reserve(total);
                request->_tempObject = body;
            }
            String* body = static_cast<String*>(request->_tempObject);
            if (body && index + len <= WEB_CONFIG_BODY_MAX_BYTES) {
                body->concat(reinterpret_cast<const char*>(data), len);
            }
        });

    httpServer->on("/api/config/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!requireManagementAuth(request)) return;
        if (!configManager.reset()) {
            request->send(500, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Could not reset configuration\"}");
            return;
        }
        request->send(
            200, "application/json",
            "{\"status\":\"ok\",\"msg\":\"Factory reset saved; reboot required. New AP credentials will be displayed on boot.\"}");
    });

    httpServer->on("/api/power", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!requireManagementAuth(request)) return;
        if (!request->hasParam("mode")) {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"missing 'mode' param\"}");
            return;
        }
        const String mode = request->getParam("mode")->value();
        if (mode == "0") {
            powerManager.setMode(POWER_FULL);
        } else if (mode == "1") {
            powerManager.setMode(POWER_BALANCED);
        } else {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"mode: 0=Full,1=Balanced\"}");
            return;
        }
        const String response =
            "{\"status\":\"ok\",\"mode\":\"" + String(powerManager.getModeName()) + "\"}";
        request->send(200, "application/json", response);
    });
    
    
    httpServer->begin();
    isRunning = true;
    
    Serial.printf("[WEB] Server started on port %d\n", WEB_SERVER_PORT);
    Serial.printf("[WEB] Dashboard: http://%s\n", getIPAddress().toString().c_str());
    
    return true;
}

void SkySweepWebServer::stop() {
    if (httpServer) {
        httpServer->end();
        delete httpServer;
        httpServer = nullptr;
    }
    if (webSocket) {
        webSocket->closeAll();
        delete webSocket;
        webSocket = nullptr;
    }
    isRunning = false;
}

void SkySweepWebServer::update() {
    if (!isRunning || !webSocket) return;
    
    // Cleanup disconnected clients
    webSocket->cleanupClients(MAX_WEBSOCKET_CLIENTS);
    
    // Broadcast system status periodically
    if (millis() - lastBroadcastTime >= WEB_BROADCAST_INTERVAL_MS) {
        lastBroadcastTime = millis();
        
        JsonDocument doc;
        doc["type"] = "status";
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        doc["clients"] = webSocket->count();
        doc["batV"] = powerManager.getBatteryVoltage();
        doc["batPct"] = powerManager.getBatteryPercent();
        doc["pwrMode"] = powerManager.getModeName();
        doc["estMin"] = powerManager.getEstimatedRuntimeMinutes();
        
        JsonArray modules = doc["modules"].to<JsonArray>();
        JsonObject subGhz = modules.add<JsonObject>();
        subGhz["name"] = "E07 / CC1101 855-925";
        subGhz["on"] = true;
        JsonObject twoPointFour = modules.add<JsonObject>();
        twoPointFour["name"] = "E28 / SX1281 2.4";
        twoPointFour["on"] = true;
        JsonObject fivePointEight = modules.add<JsonObject>();
        fivePointEight["name"] = "RX5808 5.8";
        fivePointEight["on"] = true;
        JsonObject gnss = modules.add<JsonObject>();
        gnss["name"] = "GNSS";
        gnss["on"] = true;
        JsonObject storage = modules.add<JsonObject>();
        storage["name"] = "microSD";
        storage["on"] = true;
        
        String output;
        serializeJson(doc, output);
        webSocket->textAll(output);
    }
}

void SkySweepWebServer::handleRoot(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response =
        request->beginResponse(200, "text/html", DASHBOARD_HTML_GZ, DASHBOARD_HTML_GZ_LEN);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void SkySweepWebServer::handleAPI(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["version"] = SKYSWEEP_VERSION;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
    
    doc["profile"] = "Rev C passive monitor";
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void SkySweepWebServer::handleConfig(AsyncWebServerRequest* request) {
    // Return runtime config from ConfigManager
    request->send(200, "application/json", configManager.toJSON());
}

void SkySweepWebServer::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not Found");
}

void SkySweepWebServer::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                          AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WEB] Client #%u connected from %s\n", client->id(), 
                         client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WEB] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA: {
            // Handle incoming commands from dashboard
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                // Print length-bounded rather than NUL-terminating one byte past the frame buffer.
                Serial.printf("[WEB] WS message: %.*s\n", (int)len, (const char*)data);
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void SkySweepWebServer::broadcastRFData(const char* moduleName, int rssi, bool active) {
    if (!isRunning || !webSocket || webSocket->count() == 0) return;
    
    JsonDocument doc;
    doc["type"] = "rf";
    doc["module"] = moduleName;
    doc["rssi"] = rssi;
    doc["active"] = active;
    
    String output;
    serializeJson(doc, output);
    webSocket->textAll(output);
}

void SkySweepWebServer::broadcastDroneDetection(const char* droneID, double lat, double lon, float alt) {
    if (!isRunning || !webSocket || webSocket->count() == 0) return;
    
    JsonDocument doc;
    doc["type"] = "drone";
    doc["id"] = droneID;
    doc["lat"] = lat;
    doc["lon"] = lon;
    doc["alt"] = alt;
    
    String output;
    serializeJson(doc, output);
    webSocket->textAll(output);
}

void SkySweepWebServer::broadcastActivityLevel(const char* level, const char* source) {
    if (!isRunning || !webSocket || webSocket->count() == 0) return;

    JsonDocument doc;
    doc["type"] = "activity";
    doc["level"] = level;
    doc["basis"] = String(source) + " normalized energy/RSSI; source not identified";

    String output;
    serializeJson(doc, output);
    webSocket->textAll(output);
}

void SkySweepWebServer::broadcastSystemStatus(const char* status) {
    if (!isRunning || !webSocket || webSocket->count() == 0) return;
    
    JsonDocument doc;
    doc["type"] = "system";
    doc["msg"] = status;
    
    String output;
    serializeJson(doc, output);
    webSocket->textAll(output);
}

IPAddress SkySweepWebServer::getIPAddress() const {
    return WiFi.softAPIP();
}

uint16_t SkySweepWebServer::getConnectedClients() const {
    if (webSocket) return webSocket->count();
    return 0;
}

#endif // MODULE_WEB_SERVER
