#ifndef WIFI_SERVER_MODULE_H
#define WIFI_SERVER_MODULE_H

#include "IModule.h"
#include <WiFi.h>
#include <WebServer.h>

/**
 * @brief Wi-Fi Access Point and HTTP REST/JSON Telemetry Backend Server module.
 * Creates a wireless Access Point ("Honda-CL250-AP") and serves live telemetry JSON
 * endpoints for smartphone mobile apps and Web browser dashboards.
 */
class WiFiServerModule : public IModule {
private:
    WebServer _server;
    unsigned long _lastUpdate = 0;
    SystemState* _pSystemState = nullptr;

    /**
     * @brief Handles HTTP GET request to /api/telemetry returning JSON payload.
     */
    void handleTelemetryJson();

    /**
     * @brief Handles HTTP GET request to root endpoint serving status page.
     */
    void handleRoot();

public:
    WiFiServerModule(uint16_t port = 80);
    virtual ~WiFiServerModule() {}

    bool begin() override;
    void update(SystemState& state) override;
};

#endif // WIFI_SERVER_MODULE_H
