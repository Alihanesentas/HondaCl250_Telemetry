import 'dart:typed_data';

/// Telemetry model representing binary BLE packet from Honda CL250 ESP32.
class TelemetryData {
  final double rpm;
  final int speed;
  final int coolantTemp;
  final double throttlePos;
  final double batteryVolt;
  final double leanAngle;
  final double maxLeanRight;
  final double maxLeanLeft;

  TelemetryData({
    required this.rpm,
    required this.speed,
    required this.coolantTemp,
    required this.throttlePos,
    required this.batteryVolt,
    required this.leanAngle,
    required this.maxLeanRight,
    required this.maxLeanLeft,
  });

  factory TelemetryData.initial() {
    return TelemetryData(
      rpm: 0,
      speed: 0,
      coolantTemp: 0,
      throttlePos: 0,
      batteryVolt: 0.0,
      leanAngle: 0.0,
      maxLeanRight: 0.0,
      maxLeanLeft: 0.0,
    );
  }

  /// Parses 13-byte binary packet received from BLE notification.
  factory TelemetryData.fromBinaryBuffer(Uint8List bytes) {
    if (bytes.length < 13) return TelemetryData.initial();

    try {
      final buffer = ByteData.sublistView(bytes);

      final rpm = buffer.getUint16(0, Endian.little).toDouble();
      final speed = buffer.getUint8(2);
      final coolantTemp = buffer.getInt8(3);
      final throttlePos = buffer.getUint8(4).toDouble();
      final batteryVolt = buffer.getUint16(5, Endian.little) / 1000.0;
      final leanAngle = buffer.getInt16(7, Endian.little) / 10.0;
      final maxLeanRight = buffer.getInt16(9, Endian.little) / 10.0;
      final maxLeanLeft = buffer.getInt16(11, Endian.little) / 10.0;

      return TelemetryData(
        rpm: rpm,
        speed: speed,
        coolantTemp: coolantTemp,
        throttlePos: throttlePos,
        batteryVolt: batteryVolt,
        leanAngle: leanAngle,
        maxLeanRight: maxLeanRight,
        maxLeanLeft: maxLeanLeft,
      );
    } catch (e) {
      return TelemetryData.initial();
    }
  }
}
