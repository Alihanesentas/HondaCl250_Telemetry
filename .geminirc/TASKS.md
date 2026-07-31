# PROJECT ROADMAP & TASK TRACKER

## COMPLETED TASKS
- [x] **ESP32 Firmware Modular Refactoring**: Implemented `IModule` abstract interface for all hardware subsystems.
- [x] **Honda ECU CAN Bus Driver**: Integrated TWAI CAN controller with UDS diagnostic frames ($18DA10F1 / $18DAF110).
- [x] **IMU Riding Dynamics**: Built MPU6050 I2C driver with complementary filter for roll/lean angle and peak lean tracking.
- [x] **Nextion HMI Driver**: Built Nextion UART display module (10Hz refresh rate).
- [x] **BLE GATT Telemetry Server**: Implemented BLE Server sending 12-byte packed binary notifications and receiving smartphone telematics.
- [x] **Wi-Fi SoftAP & HTTP API Server**: Added `WiFiServerModule` (`Honda-CL250-AP`) serving live JSON telemetry at `/api/telemetry`.
- [x] **Mobile Application Suite**:
  - Web Bluetooth Cockpit PWA (`mobile_app/index.html`) with gauge animations and Demo Simulator.
  - Native Flutter App (`mobile_app/flutter_app/`) with Android BLE permissions configuration.
- [x] **Documentation & Comments**: Consolidated all codebase comments in English and created `CONTEXT.md` and `README.md`.

## FUTURE EXPANSION MODULES (PLANNED)
- [ ] **GPS Logger Module**: Plug-in `GPSModule` implementing `IModule` to log latitude, longitude, and lap times.
- [ ] **TPMS Tire Pressure Module**: Plug-in `TPMSModule` to read Bluetooth/433MHz tire pressure sensors.
- [ ] **SD Card Telemetry Data Logger**: Plug-in `SDLoggerModule` to log CSV telemetry records onto SPI micro-SD card.
- [ ] **Quickshifter / Gear Position Sensor Module**: Plug-in gear calculation module based on RPM vs Speed ratio.
