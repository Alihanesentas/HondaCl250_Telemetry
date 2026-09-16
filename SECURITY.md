# Security Posture — Honda CL250 Telemetry

This is a hobby/prototype embedded telemetry project, not a certified automotive
product. This document lists what has been hardened, and what has been
**consciously accepted as an out-of-scope limitation** rather than fixed — the
lightweight, single-project equivalent of an ISO 21434 threat/risk assessment.
Nothing below is an oversight; each accepted item has a reason.

## What has been hardened

| Area | Mitigation | Where |
|---|---|---|
| Wi-Fi SoftAP | WPA2-PSK, password kept out of source control (build flag via gitignored `platformio_local.ini`), AP off by default (only enabled on BOOT-button hold or a 15s BLE-fallback timeout) | G4.1 |
| BLE telematics write | Bonding + link encryption required (`ESP_LE_AUTH_REQ_SC_BOND`, `ESP_GATT_PERM_WRITE_ENCRYPTED`); telemetry notify stream stays open/unauthenticated by design | G4.2 |
| BLE/HTTP → Nextion/JSON injection | Phone-controlled strings (song title, artist) are character-filtered before reaching a Nextion `"..."` command, and JSON-escaped before reaching the HTTP API | G4.3 |
| BLE packet integrity | Versioned wire format, receivers reject a version mismatch instead of misparsing | G3.3 |
| CAN/UDS malformed responses | Negative response codes (NRC), multi-frame ISO-TP responses, and request timeouts are all explicitly handled instead of silently corrupting state | G2.1, G2.2, G2.4 |
| Flash/NVS wear & corruption | `WiFi.persistent(false)` — the only flash-writing behavior found — disabled; the firmware otherwise performs no runtime flash/NVS writes | G1.5 |

## Accepted limitations (will not be fixed in this project)

### No secure boot
The ESP32-S3's secure boot feature is not enabled. Anyone with physical access to
the board and a JTAG/serial connection can flash arbitrary firmware.
**Accepted because:** secure boot requires burning one-time-programmable eFuses,
which is irreversible and would block this project's own iterative
flash-and-test development workflow. There is no remote attacker path to
firmware replacement (no OTA — see below), so the residual risk is "attacker
already has the device in hand," which is out of scope for a motorcycle
accessory.

### No flash encryption
Firmware and any embedded secrets (e.g., the Wi-Fi password baked in via
`AP_PASSWORD` at build time) are stored in plaintext flash and can be extracted
with physical access (UART/JTAG dump).
**Accepted because:** same physical-access precondition as secure boot. The
project's actual mitigation is keeping the password out of the *git repository*
(G4.1) and defaulting the AP to off, not defending the flash chip itself against
someone who already has the hardware.

### No physical access protection
There is no tamper detection, no case intrusion switch, no protection against
someone probing the DLC/OBD-II port, UART pins, or I2C bus directly.
**Accepted because:** this is the same trust model as the vehicle's OEM
diagnostic port itself — physical access to a motorcycle's wiring harness is
already the attacker's strongest position, with or without this telemetry unit
installed.

### No OTA (over-the-air) firmware update
Firmware can only be updated via a wired USB connection to a computer running
PlatformIO.
**Accepted because:** this removes an entire remote-attack surface class (OTA
update-server compromise, malicious firmware push) at essentially zero cost,
since this is a single-unit hobby project with no fleet-update requirement.

### BLE pairing has no MITM protection
G4.2 requires bonding and link encryption for the BLE write characteristic, but
uses `ESP_IO_CAP_NONE` ("Just Works" pairing) because the ESP32-S3 board has no
display or keypad to show/confirm a passkey. Just Works pairing encrypts the
link and blocks *opportunistic* unpaired writes, but an attacker who can
actively intercept the one-time pairing handshake (in range, at the exact
moment of first pairing) could still insert themselves.
**Accepted because:** passkey/numeric-comparison pairing modes require I/O
hardware this board doesn't have; adding a display or keypad purely for this
would be a real hardware cost for a low-probability, narrow-window attack
(the pairing handshake happens once, briefly, typically in the rider's own
garage). The BLE telemetry notify stream itself stays intentionally
unauthenticated (read-only vehicle speed/RPM is not considered sensitive).

### HTTP `/api/telemetry` has no authentication beyond the Wi-Fi password
Once connected to the SoftAP (which requires the WPA2 password, G4.1), any
device on that AP can read `/api/telemetry` with no additional login.
**Accepted because:** the endpoint is read-only telemetry (no control/write
capability exists over HTTP), and the WPA2 password is already the intended
access boundary — adding a second credential layer for read-only data on a
network the device owner controls would add complexity without a
corresponding threat it defends against.

### CAN bus / UDS requests have no authentication
Any device wired to the vehicle's CAN bus can send the same UDS requests this
firmware sends, and the firmware itself does not authenticate the ECU's
responses.
**Accepted because:** this matches the vehicle manufacturer's own diagnostic
protocol design (UDS over CAN has no authentication in this vehicle class) --
this project is a passive/read-mostly participant on a bus with no
authentication model to opt into.

## Reporting

This is a personal hobby project with no formal disclosure process. If you find
an issue, open a GitHub issue on this repository.
