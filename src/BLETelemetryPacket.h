#ifndef BLE_TELEMETRY_PACKET_H
#define BLE_TELEMETRY_PACKET_H

#include <Arduino.h>

/**
 * @brief Binary telemetry payload sent over BLE notifications.
 * Packed attribute guarantees byte alignment across different microcontrollers/mobile OS architectures.
 */
struct __attribute__((packed)) BLETelemetryPacket {
    uint16_t rpm;          // Engine RPM (0 - 15,000)
    uint8_t  speed;        // Vehicle Speed in km/h (0 - 255)
    int8_t   coolantTemp;  // Coolant Temperature in °C (-40 to 150)
    uint8_t  throttlePos;  // Throttle position percentage (0 - 100%)
    uint16_t batteryVolt;  // Battery Voltage in millivolts (e.g., 12400 = 12.4V)
    int16_t  leanAngle;    // Current roll/lean angle in tenths of a degree (e.g. 254 = 25.4°)
    int16_t  maxLeanRight; // Peak right lean angle in tenths of a degree
    int16_t  maxLeanLeft;  // Peak left lean angle in tenths of a degree
};

#endif // BLE_TELEMETRY_PACKET_H
