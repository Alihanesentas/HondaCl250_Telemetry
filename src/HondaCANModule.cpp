#include "HondaCANModule.h"

// 29-bit Extended Honda UDS ID (Target ECU 0x10, Source Tool 0xF1)
#define HONDA_UDS_REQ_29BIT   0x18DA10F1
#define HONDA_UDS_RESP_29BIT  0x18DAF110

HondaCANModule::HondaCANModule(gpio_num_t txPin, gpio_num_t rxPin) 
    : _txPin(txPin), _rxPin(rxPin) {}

bool HondaCANModule::begin() {
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(_txPin, _rxPin, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_ERR_PASS;
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
        Serial.println("[CAN SUCCESS] TWAI CAN Bus Driver Active (500 kbps).");
        delay(200);
        
        // Start UDS 29-bit Extended Session ($10 $03)
        sendFrame29(0x02, 0x10, 0x03);
        delay(50);
        return true;
    }
    Serial.println("[CAN ERROR] Failed to initialize TWAI driver!");
    return false;
}

void HondaCANModule::sendFrame29(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    twai_message_t txMsg;
    txMsg.extd = 1; // 29-bit Extended Frame
    txMsg.rtr = 0;
    txMsg.identifier = HONDA_UDS_REQ_29BIT;
    txMsg.data_length_code = 8;
    txMsg.data[0] = d0;
    txMsg.data[1] = d1;
    txMsg.data[2] = d2;
    txMsg.data[3] = d3;
    for (int i = 4; i < 8; i++) txMsg.data[i] = 0xAA;

    twai_transmit(&txMsg, pdMS_TO_TICKS(5));
}

void HondaCANModule::sendFrame11(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    // Unused on Honda 29-bit bus
}

void HondaCANModule::requestDID(uint16_t did) {
    sendFrame29(0x03, 0x22, (did >> 8) & 0xFF, did & 0xFF);
}

void HondaCANModule::update(SystemState& state) {
    unsigned long now = millis();

    // 1. TWAI Bus-Off Auto Recovery Check
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            twai_initiate_recovery();
        } else if (status.state == TWAI_STATE_STOPPED) {
            twai_start();
        }
    }

    // 2. UDS Session Keep-Alive (1000ms)
    if (now - _lastKeepAlive >= 1000) {
        _lastKeepAlive = now;
        sendFrame29(0x02, 0x3E, 0x80);
    }

    // 3. High Frequency Request (Engine RPM - 50ms / 20Hz)
    if (now - _lastFastReq >= 50) {
        _lastFastReq = now;
        requestDID(0xF40C); // Engine RPM DID
    }

    // 4. Low Frequency Sequential Requests (Speed, Temp, TPS - 200ms)
    if (now - _lastSlowReq >= 200) {
        _lastSlowReq = now;
        switch (_slowSeq) {
            case 0: requestDID(0xF40D); _slowSeq = 1; break; // Vehicle Speed
            case 1: requestDID(0xF411); _slowSeq = 2; break; // Throttle Position
            case 2: requestDID(0xF405); _slowSeq = 0; break; // Coolant Temperature
        }
    }

    // 5. Read incoming CAN frames from Honda ECU
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, 0) == ESP_OK) {
        if (rxMsg.extd && rxMsg.identifier == HONDA_UDS_RESP_29BIT && rxMsg.data[1] == 0x62) {
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
            }
        }
    }
}