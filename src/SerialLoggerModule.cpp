#include "SerialLoggerModule.h"

SerialLoggerModule::SerialLoggerModule(unsigned long printIntervalMs)
    : _printIntervalMs(printIntervalMs) {}

bool SerialLoggerModule::begin() {
    // USB Serial terminal initialized in main setup()
    return true;
}

void SerialLoggerModule::update(SystemState& state) {
    unsigned long now = millis();
    if (now - _lastPrint >= _printIntervalMs) {
        _lastPrint = now;

        Serial.printf("[TELEMETRY] RPM: %6.1f | SPEED: %3d km/h | TPS: %5.1f%% | ECT: %3d°C | BATT: %4.1fV | LEAN: %5.1f° (L:%.1f°/R:%.1f°) | BLE: %s\n",
                      state.engine.rpm,
                      state.engine.speed,
                      state.engine.throttlePos,
                      state.engine.coolantTemp,
                      state.engine.batteryVoltage,
                      state.dynamics.leanAngle,
                      state.dynamics.maxLeanLeft,
                      state.dynamics.maxLeanRight,
                      state.telematics.phoneConnected ? "CONNECTED" : "DISCONNECTED");
    }
}
