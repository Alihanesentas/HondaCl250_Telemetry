#include "HondaCANModule.h"

// 29-bit Extended Honda UDS ID (Target ECU 0x10, Source Tool 0xF1)
#define HONDA_UDS_REQ_29BIT   0x18DA10F1
#define HONDA_UDS_RESP_29BIT  0x18DAF110

// 11-bit Standard Honda/OBD2 UDS ID (Target ECU 0x7E0, Source Tool 0x7E8)
#define HONDA_UDS_REQ_11BIT   0x7E0
#define HONDA_UDS_RESP_11BIT  0x7E8

HondaCANModule::HondaCANModule(gpio_num_t txPin, gpio_num_t rxPin) 
    : _txPin(txPin), _rxPin(rxPin) {}

bool HondaCANModule::begin() {
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(_txPin, _rxPin, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_ABOVE_ERR_WARN;
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
        Serial.println("[CAN SUCCESS] TWAI CAN Bus Driver Active (500 kbps). Listening for Honda ECU...");
        delay(200);
        
        // Start UDS Extended Session ($10 $03) on both 29-bit and 11-bit IDs
        sendFrame29(0x02, 0x10, 0x03);
        sendFrame11(0x02, 0x10, 0x03);
        delay(50);
        _initialized = true;
        return true;
    }
    Serial.println("[CAN ERROR] Failed to initialize TWAI CAN bus driver! Check TX/RX pins.");
    _initialized = false;
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
    twai_message_t txMsg;
    txMsg.extd = 0; // 11-bit Standard Frame
    txMsg.rtr = 0;
    txMsg.identifier = HONDA_UDS_REQ_11BIT;
    txMsg.data_length_code = 8;
    txMsg.data[0] = d0;
    txMsg.data[1] = d1;
    txMsg.data[2] = d2;
    txMsg.data[3] = d3;
    for (int i = 4; i < 8; i++) txMsg.data[i] = 0xAA;

    twai_transmit(&txMsg, pdMS_TO_TICKS(5));
}

const char* HondaCANModule::nrcName(uint8_t nrc) {
    switch (nrc) {
        case 0x10: return "generalReject";
        case 0x11: return "serviceNotSupported";
        case 0x12: return "subFunctionNotSupported";
        case 0x13: return "incorrectMessageLengthOrInvalidFormat";
        case 0x21: return "busyRepeatRequest";
        case 0x22: return "conditionsNotCorrect";
        case 0x24: return "requestSequenceError";
        case 0x31: return "requestOutOfRange";
        case 0x33: return "securityAccessDenied";
        case 0x35: return "invalidKey";
        case 0x78: return "responsePending";
        case 0x7E: return "subFunctionNotSupportedInActiveSession";
        case 0x7F: return "serviceNotSupportedInActiveSession";
        default:   return "unknownNRC";
    }
}

void HondaCANModule::requestDID(uint16_t did) {
    // Send request using both 29-bit extended and 11-bit standard CAN headers
    sendFrame29(0x03, 0x22, (did >> 8) & 0xFF, did & 0xFF);
    sendFrame11(0x03, 0x22, (did >> 8) & 0xFF, did & 0xFF);
}

void HondaCANModule::update(SystemState& state) {
    unsigned long now = millis();

    // 1. TWAI Bus-Off Auto Recovery Check (G1.3 -- exponential backoff, event logging)
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            if (!_busOff) {
                // Just entered bus-off: log once, reset backoff, attempt immediately.
                _busOff = true;
                _busOffEventCount++;
                _recoveryBackoffMs = 1000;
                _lastRecoveryAttempt = 0;
                Serial.printf("[CAN WARNING] TWAI Bus-Off detected (event #%lu, tx_err=%d, rx_err=%d). Starting recovery...\n",
                    (unsigned long)_busOffEventCount, status.tx_error_counter, status.rx_error_counter);
            }

            if (now - _lastRecoveryAttempt >= _recoveryBackoffMs) {
                _lastRecoveryAttempt = now;
                Serial.printf("[CAN WARNING] Bus-Off recovery attempt (next retry in %lu ms if this fails)...\n",
                    _recoveryBackoffMs);
                twai_initiate_recovery();
                _recoveryBackoffMs = min(_recoveryBackoffMs * 2, RECOVERY_BACKOFF_MAX_MS);
            }
        } else if (status.state == TWAI_STATE_STOPPED) {
            // twai_initiate_recovery() lands the driver here once recovery completes.
            if (_busOff) {
                Serial.println("[CAN SUCCESS] TWAI Bus-Off recovery complete, restarting driver.");
                _busOff = false;
                _recoveryBackoffMs = 1000;
            }
            twai_start();
        } else if (status.state == TWAI_STATE_RUNNING && _busOff) {
            _busOff = false;
            _recoveryBackoffMs = 1000;
        }
    }

    // 2. UDS Session Keep-Alive (1000ms)
    if (now - _lastKeepAlive >= 1000) {
        _lastKeepAlive = now;
        sendFrame29(0x02, 0x3E, 0x80);
        sendFrame11(0x02, 0x3E, 0x80);
    }

    // 3. High Frequency Request (Engine RPM - 50ms / 20Hz)
    if (now - _lastFastReq >= 50) {
        _lastFastReq = now;
        requestDID(0xF40C); // Engine RPM DID
    }

    // 4. Low Frequency Sequential Requests (Speed, Temp, TPS, Batt Volt - 200ms)
    if (now - _lastSlowReq >= 200) {
        _lastSlowReq = now;
        switch (_slowSeq) {
            case 0: requestDID(0xF40D); _slowSeq = 1; break; // Vehicle Speed
            case 1: requestDID(0xF411); _slowSeq = 2; break; // Throttle Position
            case 2: requestDID(0xF405); _slowSeq = 3; break; // Coolant Temperature
            case 3: requestDID(0xF442); _slowSeq = 0; break; // Control Module Battery Voltage
        }
    }

    // 5. Read incoming CAN frames from Honda ECU
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, 0) == ESP_OK) {
        // Check if response comes from 29-bit or 11-bit UDS frame
        bool isUDSResponse = (rxMsg.identifier == HONDA_UDS_RESP_29BIT || rxMsg.identifier == HONDA_UDS_RESP_11BIT);
        
        if (isUDSResponse && rxMsg.data[1] == 0x62) {
            uint16_t did = (rxMsg.data[2] << 8) | rxMsg.data[3];
            switch (did) {
                case 0xF40C: // Engine RPM
                    state.engine.rpm = ((rxMsg.data[4] << 8) | rxMsg.data[5]) / 4.0f;
                    state.engine.rpmUpdatedMs = now;
                    break;
                case 0xF40D: // Vehicle Speed (km/h)
                    state.engine.speed = rxMsg.data[4];
                    state.engine.speedUpdatedMs = now;
                    break;
                case 0xF405: // Coolant Temperature (°C)
                    state.engine.coolantTemp = rxMsg.data[4] - 40;
                    state.engine.coolantTempUpdatedMs = now;
                    break;
                case 0xF411: // Throttle Position (%)
                    state.engine.throttlePos = (rxMsg.data[4] * 100.0f) / 255.0f;
                    state.engine.throttlePosUpdatedMs = now;
                    break;
                case 0xF442: // Battery Voltage (mV / V)
                    if (rxMsg.data_length_code >= 6) {
                        uint16_t rawVolt = (rxMsg.data[4] << 8) | rxMsg.data[5];
                        state.engine.batteryVoltage = rawVolt > 500 ? (rawVolt / 1000.0f) : (rxMsg.data[4] / 10.0f);
                    } else {
                        state.engine.batteryVoltage = rxMsg.data[4] / 10.0f;
                    }
                    state.engine.batteryVoltageUpdatedMs = now;
                    break;
            }
        } else if (isUDSResponse && rxMsg.data[1] == 0x7F && rxMsg.data_length_code >= 4) {
            // G2.1 -- Negative response: [PCI][0x7F][echoed SID][NRC]. The ECU does not
            // echo back which DID triggered this, so on 0x78 (responsePending) both
            // request timers are pushed out uniformly rather than retrying immediately.
            // G2.2's per-request state machine will correlate this to the exact DID.
            uint8_t echoedSid = rxMsg.data[2];
            uint8_t nrc = rxMsg.data[3];
            _nrcCount++;

            if (nrc == 0x78) {
                _lastFastReq = now;
                _lastSlowReq = now;
                Serial.printf("[UDS] NRC 0x78 responsePending for SID 0x%02X -- extending wait, not retrying yet.\n", echoedSid);
            } else {
                Serial.printf("[UDS WARNING] Negative response: SID=0x%02X NRC=0x%02X (%s) [total NRCs=%lu]\n",
                    echoedSid, nrc, nrcName(nrc), (unsigned long)_nrcCount);
            }
        }
    }
}