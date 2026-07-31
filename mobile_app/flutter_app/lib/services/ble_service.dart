import 'dart:async';
import 'dart:typed_data';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../models/telemetry_data.dart';

class BleService {
  static const String serviceUuidStr = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  static const String txCharUuidStr   = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  static const String rxCharUuidStr   = "828919fe-e41c-40ee-b4c6-2c974c2d3345";

  BluetoothDevice? _targetDevice;
  BluetoothCharacteristic? _rxCharacteristic;

  final StreamController<TelemetryData> _telemetryStreamController = StreamController<TelemetryData>.broadcast();
  Stream<TelemetryData> get telemetryStream => _telemetryStreamController.stream;

  final StreamController<bool> _connectionStateController = StreamController<bool>.broadcast();
  Stream<bool> get connectionStream => _connectionStateController.stream;

  Future<void> connectToBike() async {
    // Start scanning for Honda CL250 BLE server
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

    FlutterBluePlus.scanResults.listen((results) async {
      for (ScanResult r in results) {
        if (r.device.platformName.contains("Honda-CL250")) {
          await FlutterBluePlus.stopScan();
          _targetDevice = r.device;

          await _targetDevice!.connect();
          _connectionStateController.add(true);

          List<BluetoothService> services = await _targetDevice!.discoverServices();
          for (var service in services) {
            if (service.uuid.toString().toLowerCase() == serviceUuidStr.toLowerCase()) {
              for (var characteristic in service.characteristics) {
                if (characteristic.uuid.toString().toLowerCase() == txCharUuidStr.toLowerCase()) {
                  await characteristic.setNotifyValue(true);
                  characteristic.onValueReceived.listen((value) {
                    final data = TelemetryData.fromBinaryBuffer(Uint8List.fromList(value));
                    _telemetryStreamController.add(data);
                  });
                }
                if (characteristic.uuid.toString().toLowerCase() == rxCharUuidStr.toLowerCase()) {
                  _rxCharacteristic = characteristic;
                }
              }
            }
          }
          break;
        }
      }
    });
  }

  Future<void> syncTelematics(String songTitle, String artistName, int navDist) async {
    if (_rxCharacteristic != null) {
      final payload = "SONG:$songTitle|ARTIST:$artistName|DIST:$navDist|ICON:1";
      await _rxCharacteristic!.write(payload.codeUnits);
    }
  }

  Future<void> disconnect() async {
    if (_targetDevice != null) {
      await _targetDevice!.disconnect();
      _connectionStateController.add(false);
    }
  }
}
