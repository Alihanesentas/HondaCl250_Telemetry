#include "NextionModule.h"

NextionModule::NextionModule(HardwareSerial& serial, int8_t rxPin, int8_t txPin) 
    : _serial(serial), _rxPin(rxPin), _txPin(txPin) {}

bool NextionModule::begin() {
    // HardwareSerial::begin(baud, config, rxPin, txPin)
    _serial.begin(115200, SERIAL_8N1, _rxPin, _txPin);
    return true;
}

void NextionModule::sendEndCmd() {
    _serial.write(0xFF);
    _serial.write(0xFF);
    _serial.write(0xFF);
}

void NextionModule::setVal(const char* name, int32_t val) {
    _serial.printf("%s.val=%d", name, val);
    sendEndCmd();
}

void NextionModule::setTxt(const char* name, const char* text) {
    _serial.printf("%s.txt=\"%s\"", name, text);
    sendEndCmd();
}

void NextionModule::update(SystemState& state) {
    unsigned long now = millis();
    // Refresh Nextion display at 10Hz (every 100ms) to avoid over-saturating UART bandwidth
    if (now - _lastRender >= 100) {
        _lastRender = now;

        // Engine telemetry
        setVal("n_rpm", (int32_t)state.engine.rpm);
        setVal("n_speed", state.engine.speed);
        setVal("n_temp", state.engine.coolantTemp);
        setVal("n_tps", (int32_t)state.engine.throttlePos);
        setVal("n_volt", (int32_t)(state.engine.batteryVoltage * 10.0f)); // e.g. 12.4V -> 124

        // Vehicle dynamics telemetry
        setVal("n_lean", (int32_t)state.dynamics.leanAngle);

        // Telematics / Smartphone integration (optional Nextion UI widgets)
        if (state.telematics.phoneConnected) {
            setTxt("t_song", state.telematics.songTitle);
            setVal("n_navdist", state.telematics.navDistance);
        }
    }
}