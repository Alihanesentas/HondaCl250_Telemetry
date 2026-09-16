#include "WiFiServerModule.h"

// Access Point Credentials
#define AP_SSID "Honda-CL250-AP"
#define AP_PASS "HondaCL250"

WiFiServerModule::WiFiServerModule(uint16_t port)
    : _server(port) {}

bool WiFiServerModule::begin() {
    // G1.5: AP_SSID/AP_PASS are compile-time constants -- nothing to persist across
    // reboots. Without this, WiFi.mode()/softAP() write the config to NVS flash on
    // every single boot (Arduino-ESP32 default), which is unnecessary flash wear and
    // a needless window for a sudden power-cut mid-write to corrupt the NVS partition.
    WiFi.persistent(false);

    // Configure ESP32 as Wi-Fi Access Point (SoftAP)
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASS);
    
    if (!apStarted) {
        return false;
    }

    // Configure HTTP route endpoints
    _server.on("/", std::bind(&WiFiServerModule::handleRoot, this));
    _server.on("/api/telemetry", std::bind(&WiFiServerModule::handleTelemetryJson, this));

    _server.begin();
    _initialized = true;
    return true;
}

void WiFiServerModule::handleRoot() {
    String html = "<html><head><title>Honda CL250 Telemetry AP</title></head>";
    html += "<body style='background:#0B0E14; color:#00F0FF; font-family:sans-serif; text-align:center; padding:50px;'>";
    html += "<h1>Honda CL250 Telemetry Backend Server</h1>";
    html += "<p style='color:#FFF;'>Access Point Active. Endpoint: <a href='/api/telemetry' style='color:#FFB800;'>/api/telemetry</a></p>";
    html += "</body></html>";
    _server.send(200, "text/html", html);
}

void WiFiServerModule::handleTelemetryJson() {
    if (!_pSystemState) {
        _server.send(500, "application/json", "{\"error\":\"State not bound\"}");
        return;
    }

    const EngineData& e = _pSystemState->engine;
    const DynamicsData& d = _pSystemState->dynamics;
    const TelematicsData& t = _pSystemState->telematics;

    String json = "{";
    json += "\"rpm\":" + String(e.rpm, 1) + ",";
    json += "\"speed\":" + String(e.speed) + ",";
    json += "\"coolantTemp\":" + String(e.coolantTemp) + ",";
    json += "\"throttlePos\":" + String(e.throttlePos, 1) + ",";
    json += "\"batteryVoltage\":" + String(e.batteryVoltage, 2) + ",";
    json += "\"leanAngle\":" + String(d.leanAngle, 1) + ",";
    json += "\"maxLeanLeft\":" + String(d.maxLeanLeft, 1) + ",";
    json += "\"maxLeanRight\":" + String(d.maxLeanRight, 1) + ",";
    json += "\"phoneConnected\":" + String(t.phoneConnected ? "true" : "false") + ",";
    json += "\"songTitle\":\"" + String(t.songTitle) + "\",";
    json += "\"artistName\":\"" + String(t.artistName) + "\"";
    json += "}";

    // Set CORS headers for Web dashboards
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send(200, "application/json", json);
}

void WiFiServerModule::update(SystemState& state) {
    _pSystemState = &state;
    _server.handleClient();
}
