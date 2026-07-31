# PROJECT CONTEXT: HONDA CL250 TELEMETRY & MOBILE SYSTEM

## 1. HARDWARE SPECIFICATIONS
- **Target Vehicle:** 2024 Honda CL250 Motorcycle (Euro-5 Keihin ECU)
- **Main MCU:** ESP32-S3 DevKit C-1
- **CAN Physical Layer:** TWAI CAN Transceiver (TX: GPIO 4, RX: GPIO 5)
- **IMU Sensor:** MPU6050 (I2C @ 0x68, SDA: GPIO 1, SCL: GPIO 2)
- **Display:** Nextion HMI UART Display (Serial2 @ 115200 baud, TX: GPIO 17, RX: GPIO 18)
- **Wireless Interfaces:** Dual-mode BLE GATT Server + Wi-Fi SoftAP (`Honda-CL250-AP` @ 192.168.4.1)

## 2. CAN/UDS PROTOCOL SPECIFICATION (ISO-14229)
- **Bus Speed:** 500 kbps Extended (29-bit) CAN
- **Request ID:** `0x18DA10F1` | **Response ID:** `0x18DAF110`
- **Handshake:** Session Control `0x10 0x03` | Keep-Alive Tester Present `0x3E 0x80`
- **Active DIDs:**
  - `0xF40C`: Engine RPM (`RPM = ((Byte4 << 8) | Byte5) / 4.0`)
  - `0xF40D`: Vehicle Speed (`Speed = Byte4` km/h)
  - `0xF405`: Coolant Temp (`Temp = Byte4 - 40` °C)
  - `0xF411`: Throttle Position (`TPS = (Byte4 * 100) / 255` %)
  - `0xF442`: Battery Voltage (`Voltage = ((Byte4 << 8) | Byte5) / 1000.0` V)

## 3. SOFTWARE ARCHITECTURE
- **Modular OOP C++**: `IModule` interface with polymorphic `begin()` and `update(SystemState& state)`.
- **Central Bus**: Thread-safe `SystemState` struct containing `EngineData`, `DynamicsData`, and `TelematicsData`.
- **Nextion Interface**: `n_rpm`, `n_speed`, `n_temp`, `n_tps`, `n_volt`, `n_lean`, `t_song`, `n_navdist`.
- **BLE Payload**: 12-byte packed binary struct (`BLETelemetryPacket`) over characteristic `beb5483e-36e1-4688-b7f5-ea07361b26a8`.
- **Mobile Suite**: Flutter App (`mobile_app/flutter_app/`) + Web Bluetooth Cockpit PWA (`mobile_app/index.html`).