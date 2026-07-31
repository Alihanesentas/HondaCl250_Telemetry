#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>

/**
 * @brief Engine and ECU telemetry metrics retrieved over CAN bus.
 */
struct EngineData {
    float rpm = 0.0f;
    uint8_t speed = 0;
    int16_t coolantTemp = 0;
    float throttlePos = 0.0f;
    float batteryVoltage = 0.0f;
};

/**
 * @brief Vehicle riding dynamics calculated via IMU sensor.
 */
struct DynamicsData {
    float leanAngle = 0.0f;
    float maxLeanRight = 0.0f;
    float maxLeanLeft = 0.0f;
};

/**
 * @brief Smartphone / BLE telematics data (Music & Navigation).
 */
struct TelematicsData {
    char songTitle[32] = "Not Connected";
    char artistName[32] = "N/A";
    uint16_t navDistance = 0;
    uint8_t navIconID = 0;
    bool phoneConnected = false;
};

/**
 * @brief Global System State container shared across all modules.
 */
struct SystemState {
    EngineData engine;
    DynamicsData dynamics;
    TelematicsData telematics;
};

#endif // SYSTEM_STATE_H
