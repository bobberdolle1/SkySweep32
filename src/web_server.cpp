#include "web_server.h"

#ifdef MODULE_WEB_SERVER

#include <Update.h>
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
    strncpy(config.ssid, WIFI_AP_SSID, sizeof(config.ssid));
    strncpy(config.password, WIFI_AP_PASSWORD, sizeof(config.password));
    config.apMode = true;
}

SkySweepWebServer::~SkySweepWebServer() {
    stop();
}

// Tracks whether the most recent OTA upload actually wrote & verified firmware,
// so the POST response never reports success (and reboots) on an empty/failed
// upload. Update.hasError() alone is false in both the success and never-started
// cases, so a dedicated flag set during the upload is required.
static bool s_otaSuccess = false;

bool SkySweepWebServer::begin(bool accessPointMode) {
    config.apMode = accessPointMode;
    
    // Setup WiFi
    if (config.apMode) {
        #ifdef MODULE_REMOTE_ID
        // AP+STA mode for simultaneous Remote ID scanning
        WiFi.mode(WIFI_AP_STA);
        #else
        WiFi.mode(WIFI_AP);
        #endif
        
        WiFi.softAP(config.ssid, config.password, WIFI_AP_CHANNEL, 0, WIFI_MAX_CLIENTS);
        Serial.printf("[WEB] AP started: %s (IP: %s)\n", config.ssid, WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.ssid, config.password);
        
        uint8_t attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WEB] WiFi connection failed, falling back to AP mode");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(config.ssid, config.password);
        }
    }
    
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
    httpServer->on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!SD.exists("/logs")) {
            request->send(200, "application/json", "[]");
            return;
        }
        File root = SD.open("/logs");
        if (!root || !root.isDirectory()) {
            request->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Could not open logs directory\"}");
            return;
        }
        String json = "[";
        File file = root.openNextFile();
        bool first = true;
        while (file) {
            if (!file.isDirectory()) {
                if (!first) json += ",";
                json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
                first = false;
            }
            file.close();
            file = root.openNextFile();
        }
        root.close();
        json += "]";
        request->send(200, "application/json", json);
    });
    
    httpServer->on("/api/logs/download", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("file")) {
            String filename = request->getParam("file")->value();
            // Reject path-traversal / separator characters before building the path.
            if (filename.indexOf("..") >= 0 || filename.indexOf('/') >= 0 ||
                filename.indexOf('\\') >= 0 || filename.indexOf('\r') >= 0 ||
                filename.indexOf('\n') >= 0) {
                request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid filename\"}");
                return;
            }
            String filepath = "/logs/" + filename;
            if (SD.exists(filepath)) {
                AsyncWebServerResponse *response = request->beginResponse(SD, filepath, "text/plain");
                response->addHeader("Content-Disposition", "attachment; filename=" + filename);
                request->send(response);
            } else {
                request->send(404, "application/json", "{\"status\":\"error\",\"msg\":\"File not found\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Missing 'file' param\"}");
        }
    });
    #endif
    
    httpServer->onNotFound([this](AsyncWebServerRequest* request) {
        this->handleNotFound(request);
    });
    
    // --- OTA Update Endpoint ---
    httpServer->on("/api/ota", HTTP_POST, 
        // Response handler (after upload)
        [](AsyncWebServerRequest* request) {
            bool success = s_otaSuccess;  // only true when firmware was actually written & verified
            AsyncWebServerResponse* response = request->beginResponse(
                success ? 200 : 500, "application/json",
                success ? "{\"status\":\"ok\",\"msg\":\"Rebooting...\"}"
                        : "{\"status\":\"error\",\"msg\":\"Update failed\"}"
            );
            response->addHeader("Connection", "close");
            request->send(response);
            if (success) {
                delay(1000);
                ESP.restart();
            }
        },
        // Upload handler (chunk by chunk)
        [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                s_otaSuccess = false;
                Serial.printf("[OTA] Starting update: %s (%u bytes)\n", filename.c_str(), request->contentLength());
                if (!Update.begin(request->contentLength(), U_FLASH)) {
                    Update.printError(Serial);
                    return;
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                    Update.abort();
                    return;
                }
            }
            if (final) {
                if (Update.end(true)) {
                    s_otaSuccess = true;
                    Serial.printf("[OTA] Update success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
    
    // --- Runtime Config API ---
    httpServer->on("/api/config", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},  // body handler below
        nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Accumulate the body across chunks instead of writing one byte past the
            // framework buffer (data[len]=0) and instead of assuming a single chunk.
            static String body;
            if (index == 0) body = "";
            if (total <= 8192) body.concat((const char*)data, len);
            if (index + len == total) {
                if (configManager.fromJSON(body.c_str())) {
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    Serial.println("[CONFIG] Updated via web API");
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
                }
                body = "";
            }
        }
    );
    
    httpServer->on("/api/config/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        configManager.reset();
        request->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Config reset to defaults\"}");
    });
    
    // --- Power Management API ---
    httpServer->on("/api/power", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("mode")) {
            int mode = request->getParam("mode")->value().toInt();
            if (mode >= 0 && mode <= 3) {
                powerManager.setMode((PowerMode)mode);
                String resp = "{\"status\":\"ok\",\"mode\":\"" + String(powerManager.getModeName()) + "\"}";
                request->send(200, "application/json", resp);
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"mode: 0=Full,1=Balanced,2=Low,3=Sleep\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"missing 'mode' param\"}");
        }
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
        JsonObject remoteId = modules.add<JsonObject>();
        remoteId["name"] = "Remote ID (experimental)";
        remoteId["on"] = true;
        
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
    if (config.apMode) {
        return WiFi.softAPIP();
    }
    return WiFi.localIP();
}

uint16_t SkySweepWebServer::getConnectedClients() const {
    if (webSocket) return webSocket->count();
    return 0;
}

#endif // MODULE_WEB_SERVER
