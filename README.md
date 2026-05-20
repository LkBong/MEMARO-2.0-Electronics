# MEMARO 2.0 — Electronics

ESP32-based motor control board. Drives speed, direction, and enable signals to an ESCON motor driver and reads current and speed feedback via ADC. Two firmware modes: continuous manual operation and timed torque test with CSV logging.

## Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Pin Reference](#pin-reference)
- [Default Operation](#default-operation)
- [Torque Test](#torque-test)
- [Calibration](#calibration)
- [Components](#components)
- [Project Structure](#project-structure)
- [Future Work](#future-work)

---

## Hardware Requirements

- ESP32 NodeMCU development board
- ESCON motor driver (PWM input, direction input, enable input, AnOUT1, AnOUT2)
- 10 kΩ linear potentiometer (speed setpoint)
- 2x momentary push-buttons (enable toggle on GPIO21; direction toggle uses onboard IO0)
- ADC inputs (GPIO25, GPIO26) are 3.3V max — add a voltage divider if the driver outputs 0–5V
- Common GND between ESP32 and motor driver

Full wiring and assembly steps: [`ESP32_control/ESP_wiring.md`](ESP32_control/ESP_wiring.md)

---

## Pin Reference

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 4 | Potentiometer speed setpoint | IN (ADC) | 0–3.3V |
| 19 | PWM to motor driver | OUT | 20 kHz, 8-bit |
| 16 | Enable output | OUT | HIGH = enabled |
| 17 | Direction output | OUT | HIGH = CCW, LOW = CW |
| 21 | Enable / test-start button | IN | Internal PULLUP, active LOW |
| 0 (IO0) | Direction toggle button | IN | Onboard BOOT button |
| 25 | AnOUT1 current feedback | IN (ADC) | 0–3.3V → 0–1.0 A |
| 26 | AnOUT2 speed feedback | IN (ADC) | 0–3.3V → 0–10300 RPM |

> **GPIO18 must not be used** — it is VSPI_CLK and will reboot the board if pulled LOW.

---

## Default Operation

Firmware: `ESP32_control/ESP32_default/ESP32_control.ino`

1. Flash via Arduino IDE — board: **ESP32 Dev Module**, baud: **115200**
2. Power on — motor starts disabled, direction defaults to CCW
3. Set potentiometer to desired speed
4. Press **GPIO21** to enable the motor
5. Press **IO0** (onboard BOOT button) to toggle direction (CCW / CW)
6. Open Serial Monitor at 115200 to view live feedback:

```
En: ON  | Dir: CCW | Speed: 3124 RPM | Current: 0.412 A
```

---

## Torque Test

Firmware: `ESP32_control/ESP32_torque_test/ESP32_torque_test.ino`
Logger: `ESP32_control/ESP32_torque_test/ReadSerial.py`

The test runs in two phases (happens in succession, with 2 seconds of delay (can be fine-tuned)):
- **Up phase** (`TEST_DURATION_UP_MS`, CCW): motor runs at locked setpoint, speed and current are logged
- **Return phase** (`TEST_DURATION_DOWN_MS`, CW): motor returns to start position, no logging

### Steps

1. Flash the torque test sketch
2. Set potentiometer to the desired test speed — this is locked at test start
3. Open `ReadSerial.py` and set `COM_PORT` to match your port (Arduino IDE → Tools → Port) 
4. Run the logger:

```
python ReadSerial.py
```

The script sends a `ping` handshake and prints **"ESP32 ready"** when confirmed.

5. Press **GPIO21** — motor enables, test runs automatically through both phases
6. On completion, the ESP32 sends `stop` and the logger closes the CSV

### Output

CSV files are saved to `ESP32_control/ESP32_torque_test/data/log_MM-DD_HH-MM.csv`

| Column | Description |
|--------|-------------|
| `pc_time` | PC wall-clock timestamp (HH:MM:SS.mmm) |
| `elapsed_ms` | Milliseconds since test start |
| `speed_rpm` | Motor speed (RPM) |
| `current_a` | Motor current (A) |

Only the up phase is logged. The return phase runs silently.

---

## Calibration

Key constants at the top of each `.ino` file:

| Constant | Default | Firmware | Purpose |
|----------|---------|----------|---------|
| `MAX_CURRENT` | 1.0 A | both | AnOUT1 full-scale current |
| `MAX_RPM` | 10300 | both | AnOUT2 full-scale RPM |
| `TEST_DURATION_UP_MS` | 10000 ms | torque test | Up-phase (logged) duration |
| `TEST_DURATION_DOWN_MS` | 8000 ms | torque test | Return-phase duration |
| `DEBOUNCE_MS` | 50 ms | both | Button debounce window |
| PWM range | 25–179 | both | `map()` limits in `pwmSend()` (~10–70% duty) |

Update `MAX_RPM` and `MAX_CURRENT` to match your motor driver's AnOUT full-scale voltage spec.

---

## Components

KiCAD library files for the ESP32 NodeMCU are in `Components/ESP32_NODEMCU/`:

| File | Purpose |
|------|---------|
| `ESP32_NODEMCU.kicad_sym` | Schematic symbol |
| `MODULE_ESP32_NODEMCU.kicad_mod` | PCB footprint |
| `ESP32_NODEMCU.step` | 3D model |
| `how-to-import.htm` | SnapEDA import instructions |

To import: KiCAD → Preferences → Manage Symbol Libraries (or Footprint Libraries) → add the `Components/ESP32_NODEMCU/` path.

---

## Project Structure

```
MEMARO-2.0-Electronics/
├── Components/
│   └── ESP32_NODEMCU/          KiCAD symbol, footprint, 3D model, import guide
├── ESP32_control/
│   ├── ESP32_default/
│   │   └── ESP32_control.ino   Continuous manual operation firmware (backup)
│   ├── ESP32_torque_test/
│   │   ├── ESP32_torque_test.ino  Timed torque test firmware
│   │   ├── ReadSerial.py          PC-side CSV logger (pySerial)
│   │   ├── LoggingSketch.ino      Reference sketch for serial → CSV pattern
│   │   └── data/                  Test run CSVs (git-ignored)
│   └── ESP_wiring.md           Full wiring and assembly guide
├── Electronics.kicad_pcb       KiCAD PCB layout
└── README.md                   This file
```

---

## Future Work

- Bluetooth speed setpoint (replace potentiometer and physical buttons)
- Complete KiCAD PCB layout
- Dedicated test-start button on a separate GPIO (currently shares GPIO21 with enable)
