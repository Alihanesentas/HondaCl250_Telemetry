#ifndef HONDA_CAN_MODULE_H
#define HONDA_CAN_MODULE_H

#include "IModule.h"
#include "driver/twai.h"

/**
 * @brief Module handling CAN bus communications with Honda ECU via ESP32 TWAI driver.
 * Supports 29-bit Extended Honda UDS and 11-bit Standard OBD2 fallback queries.
 */
class HondaCANModule : public IModule {
private:
    gpio_num_t _txPin;
    gpio_num_t _rxPin;
    bool _initialized = false;
    unsigned long _lastKeepAlive = 0;
    unsigned long _lastFastReq = 0;
    unsigned long _lastSlowReq = 0;
    uint8_t _slowSeq = 0;

    // G1.3 -- Bus-off recovery state. Recovery attempts back off exponentially
    // (1s, 2s, 4s ... capped at 30s) instead of hammering twai_initiate_recovery()
    // every single loop pass while the bus stays off.
    bool _busOff = false;
    unsigned long _lastRecoveryAttempt = 0;
    unsigned long _recoveryBackoffMs = 1000;
    uint32_t _busOffEventCount = 0;
    static const unsigned long RECOVERY_BACKOFF_MAX_MS = 30000;

    /**
     * @brief Transmits a 29-bit Extended CAN frame for Honda UDS queries ($18DA10F1).
     */
    void sendFrame29(uint8_t d0, uint8_t d1, uint8_t d2 = 0xAA, uint8_t d3 = 0xAA);

    /**
     * @brief Transmits an 11-bit Standard CAN frame for OBD-II queries ($7DF).
     */
    void sendFrame11(uint8_t d0, uint8_t d1, uint8_t d2 = 0x55, uint8_t d3 = 0x55);

    /**
     * @brief Transmits a ReadDataByIdentifier ($22) request for a specific DID.
     */
    void requestDID(uint16_t did);

public:
    HondaCANModule(gpio_num_t txPin, gpio_num_t rxPin);
    bool begin() override;
    void update(SystemState& state) override;
    bool isHealthy() const override { return _initialized; }
};

#endif // HONDA_CAN_MODULE_H