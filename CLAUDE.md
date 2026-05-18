# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MEMARO 2.0 is an embedded motor control system using an ESP32 microcontroller to drive a DC motor via an ESCON motor driver. The firmware uses Arduino (`.ino`) format and is developed with the Arduino IDE. PCB design is done in KiCAD.

## Development Environment

**Firmware (Arduino IDE):**
- Open `ESP32_control/ESP32_control.ino` in Arduino IDE
- Board: ESP32 Dev Module (install via Boards Manager: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`)
- Upload speed: 115200 baud; Serial monitor also at **115200 baud**
- No build system or CLI toolchain — compilation and upload are done through Arduino IDE

**PCB (KiCAD):**
- Open `Electronics.kicad_pcb` for the board layout
- Custom component library lives in `Components/ESP32_NODEMCU/` — import the `.kicad_sym` (symbol) and `.kicad_mod` (footprint) into KiCAD's library manager before editing the schematic or layout

## Architecture

### Firmware (`ESP32_control/ESP32_control.ino`)

Single-file Arduino sketch with a 50 ms control loop. All logic is in `loop()`:

1. **Speed input:** Potentiometer on GPIO4 (ADC, 16-sample averaged) → maps `[0, 4095]` to PWM duty `[0, 255]` on GPIO19 at 20 kHz / 8-bit resolution. This is a **temporary** interface; Bluetooth will replace it.
2. **Direction toggle:** IO0/BOOT button (GPIO0) → flips GPIO17 (CCW/CW signal to ESCON DI1). Uses 50 ms debounce.
3. **Enable toggle:** Button on GPIO21 → flips GPIO16 (ESCON enable). Also 50 ms debounced.
4. **Feedback monitoring:** GPIO25 → current (0–1.0 A full scale), GPIO26 → speed (0–5260 RPM full scale). Printed to Serial every loop cycle.

Key constants (top of sketch):

| Constant | Value | Meaning |
|---|---|---|
| `PWM_FREQ` | 20000 | PWM frequency in Hz |
| `PWM_RES` | 8 | PWM resolution (bits) |
| `MAX_CURRENT` | 1.0 | Full-scale current feedback (A) |
| `MAX_RPM` | 5260 | Full-scale speed feedback (RPM) |
| `AVG_SAMPLES` | 16 | ADC averaging window |
| `DEBOUNCE_MS` | 50 | Button debounce (ms) |

Full pin reference and ESCON wiring are in `ESP32_control/ESP_wiring.md`.

### PCB / Hardware

- `Electronics.kicad_pcb` — board layout (currently minimal/in progress)
- `Components/ESP32_NODEMCU/` — KiCAD symbol, footprint, and STEP 3D model for the AZ Delivery ESP32 NodeMCU board (sourced from SnapEDA)

## iOS App (`ios-app/`)

SwiftUI controller app ("MEMARO Controller") that replaces the temporary potentiometer/button bench controls with wireless BLE input and displays live motor telemetry.

### Tech Stack
- **UI:** SwiftUI, landscape-only (enforced in `Info.plist` via `UISupportedInterfaceOrientations`)
- **BLE:** CoreBluetooth — no third-party packages
- **Concurrency:** `@MainActor ObservableObject` + `@Published` throughout; delegate callbacks update `@Published` properties directly. No `PassthroughSubject`, no `AsyncStream`, no `AnyCancellable`.
- **Responsiveness:** `GeometryReader` for proportional sizing; baseline design device is iPhone 13
- **Min iOS:** 16.0

### iOS Hard Constraints — Do Not Change Without Explicit Instruction
- `@MainActor ObservableObject` + `@Published` only — no Combine chains, no `AsyncStream`, no `AnyCancellable`
- `motorCharacteristic: CBCharacteristic?` must be set to `nil` on disconnect before any reconnect attempt
- `NSBluetoothAlwaysUsageDescription` must be present in `Info.plist`

### BLE Protocol

**GATT Service UUID:** `4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

| Characteristic | UUID | Direction | Properties |
|---|---|---|---|
| Control | `BEB5483E-36E1-4688-B7F5-EA07361B26A8` | iOS → ESP32 | Write Without Response |
| Feedback | `1C95D5E3-D8F5-4D7F-8B9C-2B7A5C3F1234` | ESP32 → iOS | Notify |

**Control packet — 3 bytes, throttled to ~50 ms:**
- `x` (Int8, –100 to 100): left/right joystick axis (negative = left)
- `y` (Int8, –100 to 100): forward/backward joystick axis (negative = backward)
- `speed` (UInt8, 0–255): speed slider value, maps directly to ESP32 8-bit PWM

**Feedback packet — 4 bytes from ESP32 every ~50 ms:**
- Bytes 0–1: UInt16 big-endian — RPM (0–5260)
- Bytes 2–3: UInt16 big-endian — current in milliamps (0–1000 → 0.000–1.000 A)

**Control mapping:**
```
speed_sent = sliderValue (0–1) × joystickMagnitude (0–1) × 255
```
Joystick deflection and slider are both proportional; partial deflection reduces the sent speed.

### Landscape Layout (left → centre → right)
```
┌──────────────────────────────────────────────────────────────┐
│  ┌──────────┐    ┌────────────────────────┐    ┌─────────┐  │
│  │ Joystick │    │  3,142 RPM  │  0.412 A │    │  Speed  │  │
│  │    ○     │    │  ◉ MEMARO  [Connect]   │    │  Slider │  │
│  └──────────┘    └────────────────────────┘    └─────────┘  │
└──────────────────────────────────────────────────────────────┘
```
Joystick diameter = `min(width × 0.35, height × 0.85)`. BLE connect UX is a device picker (scan list → tap to connect).

### ESP32 BLE Firmware (not yet implemented)
The ESP32 firmware must advertise the GATT service above and implement both characteristics. The UUIDs, packet formats, and 50 ms notify interval are the contract between the two sides.

### Firmware Hard Constraints — Do Not Change Without Explicit Instruction

**Pins — never reassign:**
| GPIO | Function |
|------|----------|
| 19 | PWM speed output (20 kHz, 8-bit, `ledcAttach`/`ledcWrite` v3.x API) |
| 16 | Enable output (HIGH = enabled) |
| 17 | Direction output (HIGH = CCW, LOW = CW) |
| 25 | AnOUT1 current feedback ADC input |
| 26 | AnOUT2 speed feedback ADC input |

**Pins — remove when rewriting for BLE:**
| GPIO | Temporary function |
|------|-------------------|
| 4 | Potentiometer speed input |
| 21 | Enable toggle button |
| 0 | Direction toggle button |

**BLE library: NimBLE-Arduino only** — do not use ESP32 BLE Arduino (too heavy).

## Known Issues & Planned Work

- **PWM range bug:** Duty cycle does not reset correctly when it exceeds the valid range
- **ESCON DI2/DI3 unresponsive:** ESCON Studio shows DI2 and DI3 not responding; root cause under investigation
- **BLE firmware:** ESP32 BLE GATT server not yet implemented — must match the protocol defined above
- **Two-motor expansion:** PWM pin assignments in firmware are temporary (1 motor wired); will expand to differential drive when second motor is added
