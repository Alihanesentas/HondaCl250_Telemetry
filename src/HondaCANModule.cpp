#include "HondaCANModule.h"

// UDS Request ID for Honda ECU and Response ID from Honda ECU
#define HONDA_ECU_REQ_ID   0x18DA10F1
#define HONDA_ECU_RESP_ID  0x18DAF110

HondaCANModule::HondaCANModule(gpio_num_t txPin, gpio_num_t rxPin) 
    : _txPin(txPin), _rxPin(rxPin) {}

bool HondaCANModule::begin() {
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(_txPin, _rxPin, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_ERR_PASS;
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
        Serial.println("[CAN] TWAI Driver successfully installed & started (500 kbps).");
        delay(200);
        // Start UDS Extended Diagnostic Session ($10 $03)
        sendFrame(0x02, 0x10, 0x03);
        delay(50);
        return true;
    }
    Serial.println("[CAN ERROR] TWAI Driver failed to start!");
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

    // 1. TWAI Bus-Off Auto Recovery Check
    twai_status_info_t status_info;
    if (twai_get_status_info(&status_info) == ESP_OK) {
        if (status_info.state == TWAI_STATE_BUS_OFF) {
            Serial.println("[CAN WARNING] TWAI Bus-Off detected! Initiating automatic recovery...");
            twai_initiate_recovery();
        } else if (status_info.state == TWAI_STATE_STOPPED) {
            twai_start();
            sendFrame(0x02, 0x10, 0x03); // Re-start extended session
        }
    }

    // 2. UDS Session Keep-Alive (Tester Present - 1000ms)
    if (now - _lastKeepAlive >= 1000) {
        _lastKeepAlive = now;
        sendFrame(0x02, 0x3E, 0x80);
    }

    // 3. High Frequency Request (Engine RPM - 50ms / 20Hz)
    if (now - _lastFastReq >= 50) {
        _lastFastReq = now;
        requestDID(0xF40C); // Engine RPM DID
    }

    // 4. Low Frequency Sequential Requests (Speed, Temp, TPS, Volt - 200ms)
    if (now - _lastSlowReq >= 200) {
        _lastSlowReq = now;
        switch (_slowSeq) {
            case 0: requestDID(0xF40D); _slowSeq = 1; break; // Vehicle Speed
            case 1: requestDID(0xF411); _slowSeq = 2; break; // Throttle Position
            case 2: requestDID(0xF405); _slowSeq = 3; break; // Coolant Temperature
            case 3: requestDID(0xF442); _slowSeq = 0; break; // Battery Voltage
        }
    }

    // 5. Non-blocking Read CAN Messages and Write to SystemState
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, 0) == ESP_OK) {
        if (rxMsg.extd && rxMsg.identifier == HONDA_ECU_RESP_ID && rxMsg.data[1] == 0x62) {
            uint16_t did = (rxMsg.data[2] << 8) | rxMsg.data[3];
            switch (did) {
                case 0xF40C: // Engine RPM
                    state.engine.rpm = ((rxMsg.data[4] << 8) | rxMsg.data[5]) / 4.0f;
                    Serial.printf("[CAN RX] RPM: %.1f\n", state.engine.rpm);
                    break;
                case 0xF40D: // Vehicle Speed (km/h)
                    state.engine.speed = rxMsg.data[4];
                    Serial.printf("[CAN RX] SPEED: %d km/h\n", state.engine.speed);
                    break;
                case 0xF405: // Coolant Temperature (°C)
                    state.engine.coolantTemp = rxMsg.data[4] - 40;
                    Serial.printf("[CAN RX] ECT: %d °C\n", state.engine.coolantTemp);
                    break;
                case 0xF411: // Throttle Position (%)
                    state.engine.throttlePos = (rxMsg.data[4] * 100.0f) / 255.0f;
                    Serial.printf("[CAN RX] TPS: %.1f %%\n", state.engine.throttlePos);
                    break;
                case 0xF442: // Battery Voltage (Volts)
                    state.engine.batteryVoltage = ((rxMsg.data[4] << 8) | rxMsg.data[5]) / 1000.0f;
                    Serial.printf("[CAN RX] BATT: %.2f V\n", state.engine.batteryVoltage);
                    break;
            }
        }
    }
}