#include "IMUModule.h"

IMUModule::IMUModule(int sdaPin, int sclPin, uint8_t addr) 
    : _sdaPin(sdaPin), _sclPin(sclPin), _addr(addr) {}

bool IMUModule::begin() {
    Wire.begin(_sdaPin, _sclPin);
    Wire.setClock(400000); // 400kHz Fast I2C Bus Speed
    
    // Wake up MPU6050 from sleep mode
    Wire.beginTransmission(_addr);
    Wire.write(0x6B); // PWR_MGMT_1 register
    Wire.write(0x00); // Set to 0 to wake up sensor
    if (Wire.endTransmission() != 0) {
        _initialized = false;
        return false;
    }

    _initialized = true;
    _lastMicro = micros();
    return true;
}

void IMUModule::update(SystemState& state) {
    if (!_initialized) return;

    // Request 14 bytes starting from ACCEL_XOUT_H (0x3B)
    Wire.beginTransmission(_addr);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) return;
    
    Wire.requestFrom(_addr, (uint8_t)14, (uint8_t)true);
    if (Wire.available() < 14) return;

    int16_t rawAccX = (Wire.read() << 8) | Wire.read();
    int16_t rawAccY = (Wire.read() << 8) | Wire.read();
    int16_t rawAccZ = (Wire.read() << 8) | Wire.read();
    Wire.read(); Wire.read(); // Skip Temperature sensor registers
    int16_t rawGyroX = (Wire.read() << 8) | Wire.read();

    unsigned long now = micros();
    float dt = (now - _lastMicro) / 1000000.0f;
    _lastMicro = now;

    // Calculate accelerometer roll angle in degrees
    float accAngle = atan2((float)rawAccY, (float)rawAccZ) * 57.2958f;
    
    // Convert raw gyro X reading to degrees/sec (FS_SEL=0 default scale +/- 250 deg/s)
    float gyroRate = rawGyroX / 131.0f;

    // Apply Complementary Filter to fuse accelerometer and gyroscope readings
    _rollAngle = 0.96f * (_rollAngle + gyroRate * dt) + 0.04f * accAngle;

    // Update global dynamics state
    state.dynamics.leanAngle = _rollAngle;
    if (_rollAngle > state.dynamics.maxLeanRight) {
        state.dynamics.maxLeanRight = _rollAngle;
    }
    if (_rollAngle < state.dynamics.maxLeanLeft) {
        state.dynamics.maxLeanLeft = _rollAngle;
    }
}