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
├── SECURITY.md                 # Hardened areas + consciously accepted limitations
├── platformio.ini              # PlatformIO config: real, mock, and native test environments
├── docs/
│   └── ble_telemetry_packet_schema.json  # Single source of truth for the BLE packet layout
├── src/                        # ESP32 C++ Source Files
│   ├── IModule.h               # Producer/consumer module interfaces
│   ├── SystemState.h           # Central telemetry data store + staleness helper
│   ├── hal/
│   │   ├── ICanBus.h           # CAN bus abstraction (HondaCANModule depends only on this)
│   │   └── TwaiCanBus.h/cpp    # Real ESP32-S3 TWAI-backed ICanBus implementation
│   ├── HondaCANModule.h/cpp    # Honda DLC/OBD2 CAN bus UDS driver (protocol logic, hardware-independent)
│   ├── MockCANModule.h/cpp     # Synthetic telemetry source (no ECU/CAN needed) for on-device testing
│   ├── IMUModule.h/cpp         # MPU6050 accelerometer/gyro lean angle driver
│   ├── NextionModule.h/cpp     # Nextion HMI display UART driver
│   ├── BLETelemetryPacket.h    # 15-byte packed binary BLE notification payload (versioned, see docs/ble_telemetry_packet_schema.json)
│   ├── BLEServerModule.h/cpp   # ESP32 BLE GATT Server & Telematics handler
│   ├── WiFiServerModule.h/cpp  # ESP32 SoftAP & HTTP REST/JSON API server
│   ├── SerialLoggerModule.h/cpp# USB Serial monitor telemetry logger
│   └── main.cpp                # Asynchronous polymorphic module execution loop
├── test/
│   ├── test_embedded/          # Runs ON the ESP32 board over USB (pio test -e esp32-s3-devkitc-1)
│   ├── test_native/            # Host-only: SystemState/BLETelemetryPacket (pio test -e native)
│   ├── test_can_protocol/      # Host-only: HondaCANModule's real UDS logic vs. MockCanBus
│   ├── mocks/MockCanBus.h      # ICanBus test double used by test_can_protocol
│   └── native_stubs/Arduino.h  # Minimal millis()/Serial/delay() stand-in for native builds
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
2. Copy `platformio_local.ini.example` to `platformio_local.ini` and set your own `AP_PASSWORD` (gitignored, never commit the real one -- see [SECURITY.md](SECURITY.md)).
3. Connect your ESP32-S3 board via USB.
4. Build and upload firmware -- there is no single default environment, so `-e` is required:
   ```bash
   pio run -e esp32-s3-devkitc-1 -t upload        # real CAN/UDS hardware
   pio run -e esp32-s3-devkitc-1-mock -t upload   # synthetic telemetry, no ECU/CAN needed
   ```
5. Open Serial Monitor at `115200` baud to observe startup diagnostic logs.

### Running Tests
```bash
pio test -e native            # host-only: SystemState, BLETelemetryPacket, HondaCANModule's
                               # UDS protocol logic against MockCanBus -- no board needed
pio test -e esp32-s3-devkitc-1 # on-device suite, needs the board connected over USB
```

### Mobile App Installation
- **Web App**: Open [`mobile_app/index.html`](file:///Users/alihanesentas/Desktop/HondaCl250_Telemetry/mobile_app/index.html) in mobile Safari/Chrome or Blueify. Tap **Connect BLE** or **Demo Simulator**.
- **Flutter App**: Navigate to `mobile_app/flutter_app` and run:
   ```bash
   flutter run
   ```

---

## ⚠️ Known Limitations

### Lean angle is not true lean angle (G6.2)
The IMU-based lean/roll angle reading has a fundamental physics limitation, not a
calibration bug: an accelerometer measures the **combined vector** of gravity and
centripetal (cornering) acceleration, not gravity alone. In a properly balanced
turn, a motorcycle leans exactly enough to align itself with that combined
vector — so the accelerometer reads close to "upright" (near 0°) even at, say,
45° of actual lean, because the complementary filter has no way to separate
"the bike is leaned over" from "the bike is accelerating sideways."

**Status:** known and unaddressed. No fix is planned until real ride data
exists — see "Planned: Block 5" below.

**What's needed to actually fix it:** the vehicle's own CAN-bus speed signal
(already read by `HondaCANModule`) can be combined with the IMU's raw
accelerometer/gyro data to estimate and subtract the centripetal component, or
a full vehicle-model Kalman filter can be built for the same purpose. Neither
approach can be designed or tuned without first capturing synchronized raw
IMU + CAN speed data from real riding — the MPU6050 has not been connected to
real hardware yet (G6.1), so this data does not exist.

**Planned: Block 5** — once G6.1 (IMU hardware bring-up) and G6.3 (real-world
BLE/Wi-Fi verification) are done at the workshop and a real riding dataset has
been logged, revisit this with either the speed-based centripetal-correction
approach or a vehicle-model Kalman filter, whichever the logged data
justifies.

### Untested components (G6.1, G6.3)
The MPU6050 IMU code has never been run against real hardware — it has not
been I2C-address-scanned, sample-rate-verified, or raw-data-logged. Similarly,
BLE and Wi-Fi have not been end-to-end verified with a real phone. Both remain
open until real hardware/workshop access is available.

---

## 📄 Documentation & Guides
- For comprehensive protocol specs, CAN DIDs, and modular extension steps, see [CONTEXT.md](file:///Users/alihanesentas/Desktop/HondaCl250_Telemetry/CONTEXT.md).
- For step-by-step development history and architecture walkthroughs, see [walkthrough.md](file:///Users/alihanesentas/.gemini/antigravity/brain/f9052cb9-d1a4-46cd-b42e-6d75033cb075/walkthrough.md).
