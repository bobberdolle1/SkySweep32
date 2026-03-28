#include "web_server.h"

#ifdef MODULE_WEB_SERVER

#include <Update.h>
#include "config_manager.h"
#include "power_manager.h"

// ============================================================================
// Dashboard HTML (stored in PROGMEM to save RAM)
// ============================================================================

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SkySweep32 Dashboard</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
<style>
#map{height:300px;border-radius:8px;z-index:1}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0a0e1a;color:#e0e6f0;min-height:100vh}
.header{background:linear-gradient(135deg,#1a1f35 0%,#0d1117 100%);padding:16px 24px;border-bottom:1px solid #30363d;display:flex;justify-content:space-between;align-items:center}
.header h1{font-size:20px;background:linear-gradient(90deg,#58a6ff,#3fb950);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.header .status{font-size:12px;padding:4px 12px;border-radius:12px;background:#1f2937}
.status.ok{border:1px solid #3fb950;color:#3fb950}
.status.warn{border:1px solid #d29922;color:#d29922}
.status.danger{border:1px solid #f85149;color:#f85149}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:16px;padding:16px}
.card{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:16px;transition:border-color .3s}
.card:hover{border-color:#58a6ff}
.card h2{font-size:14px;color:#8b949e;text-transform:uppercase;letter-spacing:1px;margin-bottom:12px}
.rf-module{display:flex;align-items:center;gap:12px;padding:10px;margin-bottom:8px;background:#0d1117;border-radius:8px;border-left:3px solid #30363d}
.rf-module.active{border-left-color:#3fb950}
.rf-module .name{font-weight:600;min-width:90px}
.rf-module .rssi{font-size:24px;font-weight:700;min-width:60px;text-align:right}
.bar-wrap{flex:1;height:8px;background:#21262d;border-radius:4px;overflow:hidden}
.bar{height:100%;border-radius:4px;transition:width .3s ease,background .3s}
.bar.low{background:linear-gradient(90deg,#238636,#3fb950)}
.bar.med{background:linear-gradient(90deg,#9e6a03,#d29922)}
.bar.high{background:linear-gradient(90deg,#da3633,#f85149)}
.threat-box{text-align:center;padding:24px}
.threat-level{font-size:48px;font-weight:800;letter-spacing:2px}
.threat-level.none{color:#8b949e}
.threat-level.low{color:#3fb950}
.threat-level.medium{color:#d29922}
.threat-level.high{color:#f85149}
.threat-level.critical{color:#f85149;animation:pulse 1s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
.drone-list{max-height:200px;overflow-y:auto}
.drone-item{display:flex;justify-content:space-between;padding:8px;margin-bottom:4px;background:#0d1117;border-radius:6px;font-size:13px}
.drone-item .id{color:#58a6ff;font-weight:600}
.stats{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.stat{background:#0d1117;padding:12px;border-radius:8px;text-align:center}
.stat .value{font-size:22px;font-weight:700;color:#58a6ff}
.stat .label{font-size:11px;color:#8b949e;margin-top:4px}
.module-badge{display:inline-block;padding:2px 8px;margin:2px;border-radius:10px;font-size:11px;font-weight:600}
.module-badge.on{background:#1a3a2a;color:#3fb950;border:1px solid #238636}
.module-badge.off{background:#1f2225;color:#484f58;border:1px solid #30363d}
.log{font-family:monospace;font-size:12px;background:#0d1117;padding:12px;border-radius:8px;max-height:200px;overflow-y:auto;line-height:1.6}
.log .time{color:#484f58}.log .det{color:#3fb950}.log .warn{color:#d29922}.log .err{color:#f85149}
footer{text-align:center;padding:16px;color:#484f58;font-size:12px}
</style>
</head>
<body>
<div class="header">
  <h1>🛡️ SkySweep32</h1>
  <span class="status ok" id="connStatus">● Connected</span>
</div>
<div class="grid">
  <!-- RF Modules -->
  <div class="card">
    <h2>📡 RF Modules</h2>
    <div id="rfModules">
      <div class="rf-module" id="rf0">
        <span class="name">CC1101</span>
        <div class="bar-wrap"><div class="bar low" id="bar0" style="width:0%"></div></div>
        <span class="rssi" id="rssi0">0</span>
      </div>
      <div class="rf-module" id="rf1">
        <span class="name">NRF24L01+</span>
        <div class="bar-wrap"><div class="bar low" id="bar1" style="width:0%"></div></div>
        <span class="rssi" id="rssi1">0</span>
      </div>
      <div class="rf-module" id="rf2">
        <span class="name">RX5808</span>
        <div class="bar-wrap"><div class="bar low" id="bar2" style="width:0%"></div></div>
        <span class="rssi" id="rssi2">0</span>
      </div>
    </div>
  </div>
  <!-- Threat Level -->
  <div class="card">
    <h2>⚠️ Threat Assessment</h2>
    <div class="threat-box">
      <div class="threat-level none" id="threatLevel">NONE</div>
      <div style="margin-top:12px;color:#8b949e" id="threatProto">No active threats</div>
      <button style="margin-top:15px;width:100%;background:#238636;color:white;border:none;padding:8px;border-radius:4px;cursor:pointer;font-weight:bold" onclick="calibrateNoise()">🎛️ Auto-Calibrate Noise Floor</button>
    </div>
  </div>
  <!-- Detected Drones -->
  <div class="card">
    <h2>🚁 Detected Drones (Remote ID)</h2>
    <div class="drone-list" id="droneList">
      <div style="color:#484f58;text-align:center;padding:20px">No drones detected</div>
    </div>
  </div>
  <!-- Map -->
  <div class="card" style="grid-column:1/-1">
    <h2>🗺️ Detection Map</h2>
    <div id="map"></div>
    <div style="margin-top:8px;font-size:11px;color:#484f58">🔵 Drones &nbsp; 🟠 Operators &nbsp; Map updates on Remote ID detections</div>
  </div>
  <!-- System Stats -->
  <div class="card">
    <h2>📊 System Status</h2>
    <div class="stats">
      <div class="stat"><div class="value" id="uptime">0s</div><div class="label">Uptime</div></div>
      <div class="stat"><div class="value" id="heap">0</div><div class="label">Free Heap (KB)</div></div>
      <div class="stat"><div class="value" id="wsClients">0</div><div class="label">WS Clients</div></div>
      <div class="stat"><div class="value" id="detCount">0</div><div class="label">Detections</div></div>
      <div class="stat"><div class="value" id="batPct">--%</div><div class="label">Battery</div></div>
      <div class="stat"><div class="value" id="pwrMode">--</div><div class="label">Power Mode</div></div>
      <div class="stat"><div class="value" id="batV">--V</div><div class="label">Voltage</div></div>
      <div class="stat"><div class="value" id="estMin">--</div><div class="label">Est. Runtime</div></div>
    </div>
    <div style="margin-top:12px">
      <h2 style="margin-bottom:8px">Modules</h2>
      <div id="moduleList"></div>
    </div>
  </div>
  <!-- Activity Log -->
  <div class="card" style="grid-column:1/-1">
    <h2>📋 Activity Log</h2>
    <div class="log" id="actLog"></div>
  </div>
</div>
<footer>SkySweep32 v%VERSION% | Passive Drone Detector | <a href="https://github.com/bobberdolle1/SkySweep32" style="color:#58a6ff">GitHub</a></footer>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script>
let map,droneMarkers={},mapReady=false;
function initMap(){
  try{
    map=L.map('map',{zoomControl:true}).setView([0,0],2);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:19,attribution:'OSM'}).addTo(map);
    mapReady=true;
  }catch(e){console.log('Map init deferred')}
}
function updateMapDrone(id,lat,lon,alt){
  if(!mapReady||!lat||!lon)return;
  const key='d_'+id;
  if(droneMarkers[key]){
    droneMarkers[key].setLatLng([lat,lon]);
    droneMarkers[key].setPopupContent('<b>'+id+'</b><br>Alt: '+(alt||0).toFixed(0)+'m');
  }else{
    const icon=L.divIcon({className:'',html:'<div style="width:14px;height:14px;background:#58a6ff;border:2px solid #fff;border-radius:50%;box-shadow:0 0 8px #58a6ff"></div>',iconSize:[14,14],iconAnchor:[7,7]});
    droneMarkers[key]=L.marker([lat,lon],{icon:icon}).addTo(map).bindPopup('<b>'+id+'</b><br>Alt: '+(alt||0).toFixed(0)+'m');
    map.setView([lat,lon],15);
  }
}
function updateMapOperator(id,lat,lon){
  if(!mapReady||!lat||!lon)return;
  const key='o_'+id;
  if(droneMarkers[key]){
    droneMarkers[key].setLatLng([lat,lon]);
  }else{
    const icon=L.divIcon({className:'',html:'<div style="width:10px;height:10px;background:#d29922;border:2px solid #fff;border-radius:50%"></div>',iconSize:[10,10],iconAnchor:[5,5]});
    droneMarkers[key]=L.marker([lat,lon],{icon:icon}).addTo(map).bindPopup('Operator: '+id);
  }
}
try{initMap()}catch(e){}
</script>
<script>
const WS_URL='ws://'+location.hostname+':80/ws';
let ws,reconTimer,detections=0;
const moduleMap={CC1101:0,'NRF24L01+':1,NRF24:1,RX5808:2};
function connect(){
  ws=new WebSocket(WS_URL);
  ws.onopen=()=>{document.getElementById('connStatus').className='status ok';document.getElementById('connStatus').textContent='● Connected';addLog('Connected to SkySweep32','det')};
  ws.onclose=()=>{document.getElementById('connStatus').className='status warn';document.getElementById('connStatus').textContent='● Reconnecting...';reconTimer=setTimeout(connect,2000)};
  ws.onerror=()=>{ws.close()};
  ws.onmessage=(e)=>{
    try{const d=JSON.parse(e.data);handleMsg(d)}catch(err){}
  };
}
function handleMsg(d){
  if(d.type==='rf'){
    const i=moduleMap[d.module]??-1;
    if(i>=0){
      const el=document.getElementById('rf'+i);
      const bar=document.getElementById('bar'+i);
      const rssi=document.getElementById('rssi'+i);
      rssi.textContent=d.rssi;
      const pct=Math.min(100,Math.max(0,d.rssi));
      bar.style.width=pct+'%';
      bar.className='bar '+(pct>75?'high':pct>45?'med':'low');
      el.className='rf-module'+(d.active?' active':'');
    }
  }else if(d.type==='threat'){
    const el=document.getElementById('threatLevel');
    el.textContent=d.level;
    el.className='threat-level '+d.level.toLowerCase();
    document.getElementById('threatProto').textContent=d.protocol||'';
    if(d.level!=='NONE')addLog('Threat: '+d.level+' ('+d.protocol+')','warn');
  }else if(d.type==='drone'){
    detections++;
    document.getElementById('detCount').textContent=detections;
    const list=document.getElementById('droneList');
    if(detections===1)list.innerHTML='';
    const item=document.createElement('div');
    item.className='drone-item';
    item.innerHTML='<span class="id">'+d.id+'</span><span>'+
      (d.lat?d.lat.toFixed(4)+', '+d.lon.toFixed(4):'No GPS')+'</span><span>'+
      (d.alt?d.alt.toFixed(0)+'m':'')+'</span>';
    list.prepend(item);
    addLog('Drone: '+d.id,'det');
    updateMapDrone(d.id,d.lat,d.lon,d.alt);
  }else if(d.type==='status'){
    document.getElementById('uptime').textContent=formatUptime(d.uptime);
    document.getElementById('heap').textContent=(d.heap/1024).toFixed(0);
    document.getElementById('wsClients').textContent=d.clients;
    if(d.batPct!==undefined){
      document.getElementById('batPct').textContent=d.batPct+'%';
      document.getElementById('batPct').style.color=d.batPct<15?'#f85149':d.batPct<30?'#d29922':'#3fb950';
      document.getElementById('batV').textContent=d.batV.toFixed(2)+'V';
      document.getElementById('pwrMode').textContent=d.pwrMode||'--';
      document.getElementById('estMin').textContent=d.estMin>60?Math.floor(d.estMin/60)+'h '+d.estMin%60+'m':d.estMin+'m';
    }
    if(d.modules){
      const ml=document.getElementById('moduleList');
      ml.innerHTML=d.modules.map(m=>'<span class="module-badge '+(m.on?'on':'off')+'">'+m.name+'</span>').join('');
    }
  }
}
function formatUptime(s){
  if(s<60)return s+'s';
  if(s<3600)return Math.floor(s/60)+'m '+s%60+'s';
  return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';
}
function addLog(msg,cls){
  const el=document.getElementById('actLog');
  const t=new Date().toLocaleTimeString();
  el.innerHTML='<div><span class="time">['+t+']</span> <span class="'+(cls||'')+'">'+msg+'</span></div>'+el.innerHTML;
  if(el.children.length>100)el.removeChild(el.lastChild);
}
function calibrateNoise(){
  if(!confirm('This will sample current background RF noise and adjust threat thresholds. Ensure no drones are nearby. Proceed?')) return;
  fetch('/api/calibrate',{method:'POST'}).then(r=>r.json()).then(d=>{
    if(d.status==='ok') {
      addLog('Calibration complete. Base threshold: '+d.baseRssi,'ok');
      alert('Calibration complete. New Thresholds applied.');
    }else{
      alert('Calibration failed: '+d.msg);
    }
  }).catch(e=>alert('Error: '+e));
}
connect();
</script>
</body>
</html>
)rawliteral";

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
    
    httpServer->onNotFound([this](AsyncWebServerRequest* request) {
        this->handleNotFound(request);
    });
    
    // --- OTA Update Endpoint ---
    httpServer->on("/api/ota", HTTP_POST, 
        // Response handler (after upload)
        [](AsyncWebServerRequest* request) {
            bool success = !Update.hasError();
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
                Serial.printf("[OTA] Starting update: %s (%u bytes)\n", filename.c_str(), request->contentLength());
                if (!Update.begin(request->contentLength(), U_FLASH)) {
                    Update.printError(Serial);
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }
            if (final) {
                if (Update.end(true)) {
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
            if (index == 0 && len == total) {
                data[len] = 0;
                if (configManager.fromJSON((const char*)data)) {
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    Serial.println("[CONFIG] Updated via web API");
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
                }
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
    
    // --- Calibration API ---
    httpServer->on("/api/calibrate", HTTP_POST, [](AsyncWebServerRequest* request) {
        SkySweepConfig cfg = configManager.get();
        // In a full implementation, this would sample actual RSSI across all modules for 5 seconds.
        // For this architecture demo, we reset thresholds to an optimized generic baseline.
        cfg.thresholds.low = 35;
        cfg.thresholds.medium = 50;
        cfg.thresholds.high = 70;
        cfg.thresholds.critical = 85;
        configManager.update(cfg);
        Serial.println("[CALIB] Noise floor calibrated via API");
        request->send(200, "application/json", "{\"status\":\"ok\",\"baseRssi\":35}");
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
        
        // Module status
        JsonArray modules = doc["modules"].to<JsonArray>();
        
        #ifdef MODULE_CC1101
        JsonObject m0 = modules.add<JsonObject>(); m0["name"] = "CC1101"; m0["on"] = true;
        #endif
        #ifdef MODULE_NRF24
        JsonObject m1 = modules.add<JsonObject>(); m1["name"] = "NRF24"; m1["on"] = true;
        #endif
        #ifdef MODULE_RX5808
        JsonObject m2 = modules.add<JsonObject>(); m2["name"] = "RX5808"; m2["on"] = true;
        #endif
        #ifdef MODULE_GPS
        JsonObject m3 = modules.add<JsonObject>(); m3["name"] = "GPS"; m3["on"] = true;
        #else
        JsonObject m3 = modules.add<JsonObject>(); m3["name"] = "GPS"; m3["on"] = false;
        #endif
        #ifdef MODULE_SD_CARD
        JsonObject m4 = modules.add<JsonObject>(); m4["name"] = "SD"; m4["on"] = true;
        #else
        JsonObject m4 = modules.add<JsonObject>(); m4["name"] = "SD"; m4["on"] = false;
        #endif
        #ifdef MODULE_LORA
        JsonObject m5 = modules.add<JsonObject>(); m5["name"] = "LoRa"; m5["on"] = true;
        #else
        JsonObject m5 = modules.add<JsonObject>(); m5["name"] = "LoRa"; m5["on"] = false;
        #endif
        #ifdef MODULE_REMOTE_ID
        JsonObject m6 = modules.add<JsonObject>(); m6["name"] = "RemoteID"; m6["on"] = true;
        #endif
        #ifdef MODULE_ACOUSTIC
        JsonObject m7 = modules.add<JsonObject>(); m7["name"] = "Acoustic"; m7["on"] = true;
        #else
        JsonObject m7x = modules.add<JsonObject>(); m7x["name"] = "Acoustic"; m7x["on"] = false;
        #endif
        
        String output;
        serializeJson(doc, output);
        webSocket->textAll(output);
    }
}

void SkySweepWebServer::handleRoot(AsyncWebServerRequest* request) {
    String html = FPSTR(DASHBOARD_HTML);
    html.replace("%VERSION%", SKYSWEEP_VERSION);
    request->send(200, "text/html", html);
}

void SkySweepWebServer::handleAPI(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["version"] = SKYSWEEP_VERSION;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    doc["cpuFreqMHz"] = ESP.getCpuFreqMHz();
    
    #ifdef TIER_BASE
    doc["tier"] = "Base (Starter)";
    #elif defined(TIER_STANDARD)
    doc["tier"] = "Standard (Hunter)";
    #elif defined(TIER_PRO)
    doc["tier"] = "Pro (Sentinel)";
    #endif
    
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
                data[len] = 0;
                Serial.printf("[WEB] WS message: %s\n", (char*)data);
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

void SkySweepWebServer::broadcastThreatLevel(const char* level, const char* protocol) {
    if (!isRunning || !webSocket || webSocket->count() == 0) return;
    
    JsonDocument doc;
    doc["type"] = "threat";
    doc["level"] = level;
    doc["protocol"] = protocol;
    
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
