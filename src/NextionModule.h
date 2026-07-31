#ifndef NEXTION_MODULE_H
#define NEXTION_MODULE_H

#include "IModule.h"

/**
 * @brief Nextion HMI display driver module.
 * Sends updated telemetry values over HardwareSerial (UART) using Nextion instructions.
 */
class NextionModule : public IModule {
private:
    HardwareSerial& _serial;
    int8_t _rxPin;
    int8_t _txPin;
    unsigned long _lastRender = 0;

    /**
     * @brief Transmits Nextion instruction terminator sequence (0xFF 0xFF 0xFF).
     */
    void sendEndCmd();

    /**
     * @brief Sets numerical component property on Nextion screen.
     */
    void setVal(const char* name, int32_t val);

    /**
     * @brief Sets string component property on Nextion screen.
     */
    void setTxt(const char* name, const char* text);

public:
    NextionModule(HardwareSerial& serial, int8_t rxPin, int8_t txPin);
    bool begin() override;
    void update(SystemState& state) override;
};

#endif // NEXTION_MODULE_H