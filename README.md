# Honda CL250 Motorcycle Telemetry System

A modular, extensible embedded telemetry platform for the **Honda CL250 motorcycle** based on the **ESP32-S3**.

It reads live engine data from the motorcycle's **DLC (OBD-II) port** over CAN bus, tracks vehicle lean dynamics with an **MPU6050 IMU**, displays telemetry on a **Nextion HMI handlebar screen**, and streams data to **iOS/Android** apps over **BLE** and **Wi-Fi**.

---

## Key Features

- 🏍️ **Honda ECU CAN Bus Telemetry** — RPM, speed, coolant temperature, throttle position, and battery voltage via Honda UDS diagnostic frames over 500 kbps CAN.
- 📐 **Riding Dynamics** — real-time lean angle from the MPU6050, with peak left/right lean tracking.
- 🖥️ **Nextion HMI Display** — live gauges plus smartphone notifications (track title, turn distance) on a handlebar-mounted screen.
- 📱 **Mobile Apps** — a native Flutter app (`mobile_app/flutter_app/`) and a no-install Web Bluetooth PWA (`mobile_app/index.html`) with a built-in demo simulator.
- 📡 **Dual-Transport Streaming** — a BLE GATT server for direct phone pairing, and a Wi-Fi access point serving telemetry as JSON at `http://192.168.4.1/api/telemetry`.
- 🧩 **Modular Firmware** — each subsystem (CAN, IMU, display, BLE, Wi-Fi, logging) is an independent module behind a common interface, so new sensors are easy to add.

---

## Project Layout

- `src/` — ESP32 firmware (PlatformIO/Arduino, C++)
- `mobile_app/` — Web PWA and native Flutter app
- `docs/` — BLE packet schema
- `test/` — automated tests (two suites run on a Mac with no hardware, one runs on the board)
- `CONTEXT.md` — protocol/architecture reference
- `SECURITY.md` — what's hardened and what's a known, accepted limitation

---

## Hardware Pinout

| Function | ESP32-S3 Pin | Connects To |
|---|---|---|
| CAN TX / RX | `GPIO4` / `GPIO5` | Honda DLC / OBD-II cable (via TWAI transceiver) |
| I2C SDA / SCL | `GPIO1` / `GPIO2` | MPU6050 IMU |
| UART TX / RX | `GPIO17` / `GPIO18` | Nextion display |

---

## Getting Started

**Firmware:**
1. Open in VS Code with the PlatformIO extension.
2. Copy `platformio_local.ini.example` → `platformio_local.ini` and set your own Wi-Fi AP password (kept out of git).
3. Flash it:
   ```bash
   pio run -e esp32-s3-devkitc-1 -t upload        # real vehicle hardware
   pio run -e esp32-s3-devkitc-1-mock -t upload   # synthetic demo data, no ECU/CAN needed
   ```

**Tests** (no hardware required for these):
```bash
pio test -e native
```

**Mobile app:**
- Web: open `mobile_app/index.html` on your phone and tap Connect BLE or Demo Simulator.
- Flutter: `cd mobile_app/flutter_app && flutter run`

---

## Known Limitations

- **Lean angle isn't true lean angle yet.** An accelerometer measures gravity *and* cornering force combined — in a balanced turn it reads close to upright even at real lean angles. Fixing this needs real logged ride data (IMU + CAN speed together) to build a proper correction; that data doesn't exist yet.
- **IMU, BLE, and Wi-Fi haven't been verified against real hardware/phones yet** — the CAN/UDS telemetry path has been validated on a real bike; these haven't.

See [SECURITY.md](SECURITY.md) for the project's security posture and accepted risk tradeoffs.

## More Documentation

- [CONTEXT.md](CONTEXT.md) — CAN DIDs, protocol details, and how to extend the firmware.
- [SECURITY.md](SECURITY.md) — hardening notes and accepted limitations.
