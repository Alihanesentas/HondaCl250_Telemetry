#ifndef IMU_MODULE_H
#define IMU_MODULE_H

#include "IModule.h"
#include <Wire.h>

/**
 * @brief Inertial Measurement Unit (IMU / MPU6050) module.
 * Reads accelerometer and gyroscope data over I2C to calculate vehicle lean dynamics.
 */
class IMUModule : public IModule {
private:
    uint8_t _addr;
    int _sdaPin;
    int _sclPin;
    bool _initialized = false;
    float _rollAngle = 0.0f;
    unsigned long _lastMicro = 0;

public:
    IMUModule(int sdaPin, int sclPin, uint8_t addr = 0x68);
    bool begin() override;
    void update(SystemState& state) override;
};

#endif // IMU_MODULE_H