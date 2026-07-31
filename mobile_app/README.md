# Honda CL250 Telemetry - Mobile Applications

This directory contains two mobile application solutions for connecting to the Honda CL250 ESP32 Telemetry system via Bluetooth Low Energy (BLE):

---

## 1. Web Bluetooth Mobile PWA / Browser Dashboard (`mobile_app/`)
Can be run on any iOS (using WebBLE / Blueify browser) or Android (Chrome / Edge browser) with zero compilation!

### Features:
- **Real-Time BLE Scanner & Decoder**: Automatically parses the 12-byte binary `BLETelemetryPacket`.
- **Dynamic Cockpit UI**:
  - Animated RPM Gauge & Digital Speedometer.
  - Interactive Motorcycle Lean Angle Visualizer (chassis graphic rotates in real-time).
  - Coolant Temp, Throttle Position, and Battery Voltage meters.
- **Telematics Sync Panel**: Send song title, artist name, and navigation turn distance directly to the bike screen over BLE.
- **Built-in Demo Simulator**: Test UI gauges and lean angle animations without physical hardware.

### How to Run:
Simply open [`index.html`](file:///Users/alihanesentas/Desktop/HondaCl250_Telemetry/mobile_app/index.html) in your mobile browser or host via local web server.

---

## 2. Flutter Native Mobile App (`mobile_app/flutter_app/`)
Cross-platform native iOS & Android application built with Flutter & Dart.

### Structure:
- `lib/models/telemetry_data.dart`: Binary buffer decoder for BLE packet.
- `lib/services/ble_service.dart`: `flutter_blue_plus` scanner and characteristic stream handler.
- `lib/ui/dashboard_screen.dart`: Dark cockpit theme dashboard screen with live metric tiles and telematics controller.
- `lib/main.dart`: Entrypoint.
