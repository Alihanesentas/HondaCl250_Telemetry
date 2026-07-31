# Honda CL250 Motorcycle Telemetry System

A modular, extensible embedded telemetry platform for the **Honda CL250 motorcycle** based on the **ESP32-S3**.

The system reads live engine data from the motorcycle's **DLC (Data Link Connector) OBD-II port** over CAN bus, processes vehicle lean dynamics using an **MPU6050 IMU sensor**, displays telemetry metrics on a **Nextion HMI handlebar screen**, and streams data simultaneously to **iOS/Android Mobile Apps** via **BLE** and **Wi-Fi WebSockets/HTTP REST API**.

---

## 🌟 Key Features

- 🏍️ **Honda ECU CAN Bus Telemetry**: Reads Engine RPM, Vehicle Speed (km/h), Coolant Temperature (°C), Throttle Position (%), and Battery Voltage (V) via Honda UDS diagnostic frames over 500kbps CAN bus.
- 📐 **Riding Dynamics & Lean Angle**: Calculates real-time roll/lean angle using an MPU6050 IMU with a complementary filter, logging peak left and right lean limits.
- 🖥️ **Nextion HMI Display Integration**: Transmits live gauge values and smartphone notifications (music track title, turn distance) to a handlebar-mounted Nextion screen.
- 📱 **Mobile Application Suite**:
  - **Native Flutter Mobile App** (`mobile_app/flutter_app/`) for iOS and Android.
  - **Web Bluetooth PWA Cockpit** (`mobile_app/index.html`) runnable on mobile web browsers without compilation, featuring a built-in **Demo Simulator**.
- 📡 **Dual-Transport Backend Communication**:
  - **Bluetooth Low Energy (BLE)** GATT Server for direct mobile app telemetry streaming and telematics synchronization.
  - **Wi-Fi Access Point** (`Honda-CL250-AP`) serving live telemetry JSON API at `http://192.168.4.1/api/telemetry`.
- 🧩 **Modular C++ Architecture**: Extensible design implementing an `IModule` interface, allowing new hardware sensors (GPS, TPMS, SD loggers, quickshifter) to be added effortlessly.

---

## 📂 Project Structure

```
HondaCl250_Telemetry/
├── CONTEXT.md                  # Comprehensive technical guide & architecture specs
├── platformio.ini              # PlatformIO configuration for ESP32-S3
├── src/                        # ESP32 C++ Source Files
│   ├── IModule.h               # Abstract polymorphic module interface
│   ├── SystemState.h           # Central telemetry data store
│   ├── HondaCANModule.h/cpp    # Honda DLC/OBD2 CAN bus UDS driver (TWAI)
│   ├── IMUModule.h/cpp         # MPU6050 accelerometer/gyro lean angle driver
│   ├── NextionModule.h/cpp     # Nextion HMI display UART driver
│   ├── BLETelemetryPacket.h    # 12-byte packed binary BLE notification payload
│   ├── BLEServerModule.h/cpp   # ESP32 BLE GATT Server & Telematics handler
│   ├── WiFiServerModule.h/cpp  # ESP32 SoftAP & HTTP REST/JSON API server
│   ├── SerialLoggerModule.h/cpp# USB Serial monitor telemetry logger
│   └── main.cpp                # Asynchronous polymorphic module execution loop
└── mobile_app/                 # Mobile Application Suite
    ├── index.html              # Mobile Web Bluetooth PWA & Cockpit Dashboard
    ├── styles.css              # Dark Cockpit UI design system
    ├── app.js                  # Web BLE binary parser & gauge renderer
    └── flutter_app/            # Native Flutter Mobile Application
        ├── pubspec.yaml        # Flutter project dependencies
        ├── android/            # Android permissions & build config
        └── lib/                # Dart source code (Models, Services, Dashboard)
```

---

## 🔌 Hardware Pinout Map

| Function | ESP32-S3 Pin | Hardware Interface | Connected Device |
|---|---|---|---|
| **CAN TX** | `GPIO_NUM_4` | TWAI Transceiver TX | Honda DLC / OBD2 Cable |
| **CAN RX** | `GPIO_NUM_5` | TWAI Transceiver RX | Honda DLC / OBD2 Cable |
| **I2C SDA** | `GPIO_NUM_1` | Wire (I2C) | MPU6050 IMU SDA |
| **I2C SCL** | `GPIO_NUM_2` | Wire (I2C) | MPU6050 IMU SCL |
| **UART TX** | `GPIO_NUM_17` | HardwareSerial 2 TX | Nextion Display RX |
| **UART RX** | `GPIO_NUM_18` | HardwareSerial 2 RX | Nextion Display TX |

---

## 🛠️ Build & Flash Instructions

### ESP32-S3 Firmware (PlatformIO)
1. Open the project in VS Code with the PlatformIO extension.
2. Connect your ESP32-S3 board via USB.
3. Build and upload firmware:
   ```bash
   pio run -t upload
   ```
4. Open Serial Monitor at `115200` baud to observe startup diagnostic logs.

### Mobile App Installation
- **Web App**: Open [`mobile_app/index.html`](file:///Users/alihanesentas/Desktop/HondaCl250_Telemetry/mobile_app/index.html) in mobile Safari/Chrome or Blueify. Tap **Connect BLE** or **Demo Simulator**.
- **Flutter App**: Navigate to `mobile_app/flutter_app` and run:
   ```bash
   flutter run
   ```

---

## 📄 Documentation & Guides
- For comprehensive protocol specs, CAN DIDs, and modular extension steps, see [CONTEXT.md](file:///Users/alihanesentas/Desktop/HondaCl250_Telemetry/CONTEXT.md).
- For step-by-step development history and architecture walkthroughs, see [walkthrough.md](file:///Users/alihanesentas/.gemini/antigravity/brain/f9052cb9-d1a4-46cd-b42e-6d75033cb075/walkthrough.md).
