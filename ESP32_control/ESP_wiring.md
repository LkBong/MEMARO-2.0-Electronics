# ESP32 Motor Control — Wiring & Operation Guide

Firmware: `ESP32_control.ino`
Platform: ESP32 (NodeMCU or equivalent 3.3V dev board)

---

## Pin Reference

| GPIO | Label | Direction | Function |
|------|-------|-----------|----------|
| 12 | POT_PIN | Input (ADC) | Potentiometer wiper — sets motor speed setpoint |
| 19 | PWM_PIN | Output (PWM) | PWM speed signal to motor driver |
| 18 | EN_TOGGLE_PIN | Input | Push-button: toggles motor enable on/off |
| 17 | DIR_PIN | Output | Direction signal to motor driver |
| 0 | DIR_TOGGLE_PIN (IO0) | Input | Push-button: toggles motor direction CCW/CW |
| 25 | ANOUT1_PIN | Input (ADC) | AnOUT1 — averaged speed feedback from motor driver |
| 26 | ANOUT2_PIN | Input (ADC) | AnOUT2 — averaged current feedback from motor driver |

---

## Pin Descriptions

### GPIO12 — Potentiometer Speed Input *(temporal — testing only)*
Reads the wiper of a potentiometer as an analog voltage between 0V and 3.3V.
This voltage is sampled by the ESP32's ADC (0–4095 counts) and linearly mapped to
a PWM duty cycle of 0–100%. At 0V the motor runs at 0% duty; at 3.3V it runs at
full duty (100%).

> **Long-term:** This pin and the potentiometer are for bench testing only.
> Speed setpoint will eventually be received wirelessly via Bluetooth,
> removing the need for a physical potentiometer altogether.

---

### GPIO19 — PWM Speed Output
Outputs a 20 kHz PWM signal to the motor driver's speed input. Duty cycle is
determined by the potentiometer reading. PWM is forced to 0% whenever the motor
is disabled, regardless of the potentiometer position.

- Frequency: 20 kHz (inaudible, reduces motor heating)
- Resolution: 8-bit (0–255 counts = 0–100%)

---

### GPIO18 — Enable Toggle Button *(temporal — testing only)*
Connected to a momentary push-button (other terminal to GND). Uses internal
pull-up resistor. Each press toggles the motor between enabled and disabled
states. Edge-detected — holding the button down does not repeatedly toggle.

When disabled: PWM output is forced to 0%, motor coasts to a stop.
When enabled: PWM output follows the potentiometer.

> **Long-term:** Enable control will be managed by the Bluetooth interface.

---

### GPIO17 — Direction Output
Digital output driving the motor driver's direction input.

| Signal | Voltage | Direction |
|--------|---------|-----------|
| HIGH | 3.3V | CCW — Counter-Clockwise (dir1) |
| LOW | 0V | CW — Clockwise (dir2) |

Direction changes take effect immediately. Default state on power-up is CCW.

---

### GPIO0 — Direction Toggle Button / IO0 *(temporal — testing only)*
The IO0 pin is the onboard BOOT button present on most ESP32 dev boards —
no external button or wiring is required. Each press toggles the motor direction
between CCW and CW. Edge-detected — single press, single toggle.

> **Note:** IO0 is sampled only during `loop()`. It does not interfere with
> the ESP32 boot process once firmware is running.
>
> **Long-term:** Direction control will be managed by the Bluetooth interface.

---

### GPIO25 — AnOUT1 Speed Feedback Input
Reads the motor driver's averaged speed output voltage. Sampled over 16 ADC
readings and converted to RPM using the full-scale definition:

- 0V → 0 RPM
- 3.3V → 5260 RPM
- Linear interpolation between those points

Defined at top of firmware as `MAX_RPM 5260`. Update this value if the motor
or driver changes.

---

### GPIO26 — AnOUT2 Current Feedback Input
Reads the motor driver's averaged current output voltage. Sampled over 16 ADC
readings and converted to Amps using the full-scale definition:

- 0V → 0.000 A
- 3.3V → 1.000 A
- Linear interpolation between those points

Defined at top of firmware as `MAX_CURRENT 1.0`. Update this value if the
current range changes.

---

## Firmware-Defined Constants

Located at the top of `ESP32_control.ino` for easy adjustment:

```cpp
#define MAX_RPM       5260   // RPM corresponding to AnOUT1 at 3.3V
#define MAX_CURRENT   1.0f   // Amps corresponding to AnOUT2 at 3.3V
#define ADC_FULLSCALE 4095   // 12-bit ADC max (do not change for ESP32)
```

---

## Operation Guide

### Power-Up Behaviour
On power-up the motor starts **disabled** and direction defaults to **CCW**.
PWM output is 0% until the enable button is pressed.

### Enabling the Motor
1. Press the button on **GPIO18** once.
2. The onboard serial monitor will print `Enable: ON`.
3. The motor will begin running at the speed set by the potentiometer.
4. Press GPIO18 again to disable — motor coasts to stop.

### Adjusting Speed
Rotate the potentiometer connected to **GPIO12**:
- Fully counter-clockwise (0V) → 0% PWM → motor stopped
- Fully clockwise (3.3V) → 100% PWM → maximum speed

Speed takes effect in real time; no enable/disable cycle is needed.

### Changing Direction
1. Press **IO0** (the onboard BOOT button).
2. The serial monitor will print `Direction: CW` or `Direction: CCW`.
3. Direction change is immediate — consider disabling the motor first
   to avoid abrupt reversal under load.

### Monitoring Feedback
Open a serial monitor at **115200 baud**. Each loop cycle (~50 ms) prints:

```
En: ON  | Dir: CCW | Speed: 3124 RPM | Current: 0.412 A
```

Use this to verify the motor driver is responding correctly during testing.

---

## Assembly Steps

### Parts Required
- ESP32 dev board (NodeMCU ESP32 or equivalent)
- 10 kΩ potentiometer (linear taper)
- 2× momentary push-buttons (only one needed if using onboard IO0 button)
- Motor driver board with: PWM input, direction input, enable input, AnOUT1, AnOUT2
- Connecting wires / breadboard or PCB
- 3.3V-compatible power supply

### Step 1 — Potentiometer (GPIO12)
1. Connect one outer terminal of the potentiometer to **3.3V**.
2. Connect the other outer terminal to **GND**.
3. Connect the wiper (centre pin) to **GPIO12**.

> Ensure the potentiometer voltage never exceeds 3.3V — the ESP32 ADC pins
> are not 5V tolerant.

### Step 2 — PWM Speed Output (GPIO19)
Connect **GPIO19** to the motor driver's PWM / speed input pin.
If the motor driver is 5V logic, verify it accepts 3.3V-level signals,
or add a level shifter.

### Step 3 — Direction Output (GPIO17)
Connect **GPIO17** to the motor driver's direction input pin.
HIGH (3.3V) = CCW; LOW (0V) = CW.

### Step 4 — Enable Toggle Button (GPIO18)
1. Connect one terminal of a momentary push-button to **GPIO18**.
2. Connect the other terminal to **GND**.
No external pull-up resistor is needed — the firmware uses the ESP32's
internal pull-up.

### Step 5 — Direction Toggle Button (IO0 / GPIO0)
The IO0 pin is the onboard BOOT button — no external wiring required.
If a dedicated external button is preferred, wire it the same way as GPIO18
(one terminal to GPIO0, other to GND).

### Step 6 — Speed Feedback AnOUT1 (GPIO25)
Connect the motor driver's AnOUT1 (speed feedback) output to **GPIO25**.
Confirm the output voltage range is 0–3.3V. If the driver outputs 0–5V,
use a resistor voltage divider to scale it down before connecting.

### Step 7 — AnOUT2 Current Feedback (GPIO26)
Connect the motor driver's AnOUT2 (current feedback) output to **GPIO26**.
Same 0–3.3V requirement applies. Add a voltage divider if needed.

### Step 8 — Common Ground
Ensure the ESP32 GND and the motor driver GND share a common reference.
Without a common ground, ADC readings and logic signals will be unreliable.

### Step 9 — Upload & Verify
1. Connect the ESP32 to your PC via USB.
2. Open the project in Arduino IDE (or PlatformIO).
3. Select the correct board: **ESP32 Dev Module**.
4. Upload `ESP32_control.ino`.
5. Open serial monitor at **115200 baud**.
6. Press the GPIO18 button — confirm `Enable: ON` appears.
7. Turn the potentiometer — confirm speed feedback values increase.
8. Press IO0 — confirm direction toggles in the serial output.
