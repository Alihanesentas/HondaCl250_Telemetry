#ifndef BLE_TELEMETRY_PACKET_H
#define BLE_TELEMETRY_PACKET_H

#include <Arduino.h>

/**
 * @brief Binary telemetry payload sent over BLE notifications.
 * Packed attribute guarantees byte alignment across different microcontrollers/mobile OS architectures.
 *
 * G1.1 -- Wire format is explicitly LITTLE-ENDIAN (native to the ESP32-S3/Xtensa CPU;
 * Web Bluetooth's DataView and Dart's ByteData both default to big-endian, so every
 * consumer MUST decode with that flag set). 13 bytes total, field order below matches
 * byte offset order exactly (no padding, due to __attribute__((packed))):
 *   offset 0-1  : rpm          uint16 LE
 *   offset 2    : speed        uint8
 *   offset 3    : coolantTemp  int8
 *   offset 4    : throttlePos  uint8
 *   offset 5-6  : batteryVolt  uint16 LE
 *   offset 7-8  : leanAngle    int16 LE
 *   offset 9-10 : maxLeanRight int16 LE
 *   offset 11-12: maxLeanLeft  int16 LE
 * Mirrored in mobile_app/app.js (parseTelemetryPacket, DataView getX(offset, true))
 * and mobile_app/flutter_app/lib/models/telemetry_data.dart (fromBinaryBuffer, Endian.little).
 * Changing this layout requires updating all three places -- see G3.3 (single source of truth).
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
