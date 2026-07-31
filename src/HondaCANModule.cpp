#include "HondaCANModule.h"

// UDS Request ID for Honda ECU and Response ID from Honda ECU
#define HONDA_ECU_REQ_ID   0x18DA10F1
#define HONDA_ECU_RESP_ID  0x18DAF110

HondaCANModule::HondaCANModule(gpio_num_t txPin, gpio_num_t rxPin) 
    : _txPin(txPin), _rxPin(rxPin) {}

bool HondaCANModule::begin() {
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(_txPin, _rxPin, TWAI_MODE_NORMAL);
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
        delay(100);
        // Start UDS Extended Diagnostic Session ($10 $03)
        sendFrame(0x02, 0x10, 0x03);
        return true;
    }
    return false;
}

void HondaCANModule::sendFrame(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    twai_message_t txMsg;
    txMsg.extd = 1;
    txMsg.rtr = 0;
    txMsg.identifier = HONDA_ECU_REQ_ID;
    txMsg.data_length_code = 8;
    txMsg.data[0] = d0;
    txMsg.data[1] = d1;
    txMsg.data[2] = d2;
    txMsg.data[3] = d3;
    for (int i = 4; i < 8; i++) {
        txMsg.data[i] = 0xAA;
    }
    twai_transmit(&txMsg, pdMS_TO_TICKS(5));
}

void HondaCANModule::requestDID(uint16_t did) {
    sendFrame(0x03, 0x22, (did >> 8) & 0xFF, did & 0xFF);
}

void HondaCANModule::update(SystemState& state) {
    unsigned long now = millis();

    // 1. UDS Session Keep-Alive (Tester Present - 1000ms)
    if (now - _lastKeepAlive >= 1000) {
        _lastKeepAlive = now;
        sendFrame(0x02, 0x3E, 0x80);
    }

    // 2. High Frequency Request (Engine RPM - 50ms / 20Hz)
    if (now - _lastFastReq >= 50) {
        _lastFastReq = now;
        requestDID(0xF40C); // Engine RPM DID
    }

    // 3. Low Frequency Sequential Requests (Speed, Temp, TPS, Volt - 200ms)
    if (now - _lastSlowReq >= 200) {
        _lastSlowReq = now;
        switch (_slowSeq) {
            case 0: requestDID(0xF40D); _slowSeq = 1; break; // Vehicle Speed
            case 1: requestDID(0xF405); _slowSeq = 2; break; // Coolant Temperature
            case 2: requestDID(0xF411); _slowSeq = 3; break; // Throttle Position
            case 3: requestDID(0xF442); _slowSeq = 0; break; // Battery Voltage
        }
    }

    // 4. Process incoming CAN messages and write parsed values to SystemState
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, pdMS_TO_TICKS(1)) == ESP_OK) {
        if (rxMsg.extd && rxMsg.identifier == HONDA_ECU_RESP_ID && rxMsg.data[1] == 0x62) {
            uint16_t did = (rxMsg.data[2] << 8) | rxMsg.data[3];
            switch (did) {
                case 0xF40C: // Engine RPM
                    state.engine.rpm = ((rxMsg.data[4] << 8) | rxMsg.data[5]) / 4.0f;
                    break;
                case 0xF40D: // Vehicle Speed (km/h)
                    state.engine.speed = rxMsg.data[4];
                    break;
                case 0xF405: // Coolant Temperature (°C)
                    state.engine.coolantTemp = rxMsg.data[4] - 40;
                    break;
                case 0xF411: // Throttle Position (%)
                    state.engine.throttlePos = (rxMsg.data[4] * 100.0f) / 255.0f;
                    break;
                case 0xF442: // Battery Voltage (Volts)
                    state.engine.batteryVoltage = ((rxMsg.data[4] << 8) | rxMsg.data[5]) / 1000.0f;
                    break;
            }
        }
    }
}