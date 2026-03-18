// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT  1.0f    // Amps at 3.3V (AnOUT1 full scale)
#define MAX_RPM      5260    // RPM at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Pin definitions ───────────────────────────────────────────────────────────
// NOTE: GPIO12 (potentiometer) is temporal — for bench testing only.
//       Long-term, speed setpoint should be received via Bluetooth.
#define POT_PIN      4  // Potentiometer ADC input (0–3.3V → 0–100% PWM)

#define PWM_PIN     19  // PWM speed control output to motor driver
#define DIR_PIN     17  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define EN_OUT_PIN  16  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define ANOUT1_PIN  25  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN  26  // AnOUT2: speed feedback ADC input

// NOTE: GPIO21 (enable toggle) and GPIO0 (direction toggle) are temporal —
//       physical buttons for bench testing only.
//       GPIO18 avoided — VSPI_CLK, causes reboot when pulled LOW on most boards.
#define EN_TOGGLE_PIN  21  // Button input: toggles motor enable on each press
#define DIR_TOGGLE_PIN  0  // IO0 button input: toggles motor direction on each press

// ── PWM config ────────────────────────────────────────────────────────────────
// Using ESP32 Arduino core v3.x LEDC API (ledcAttach / ledcWrite by pin)
#define PWM_FREQ  20000  // 20 kHz
#define PWM_RES   8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── State ─────────────────────────────────────────────────────────────────────
bool motorEnabled  = false;
bool dirCCW        = true;   // true = CCW (dir1), false = CW (dir2)

#define DEBOUNCE_MS  50  // ms to wait before registering a button press

bool lastEnBtn  = HIGH;
bool lastDirBtn = HIGH;

unsigned long lastEnTime  = 0;
unsigned long lastDirTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
int adcAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return sum / AVG_SAMPLES;
}

void setup() {
  Serial.begin(115200);

  pinMode(EN_TOGGLE_PIN,  INPUT_PULLUP);
  pinMode(DIR_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(DIR_PIN,    OUTPUT);
  pinMode(EN_OUT_PIN, OUTPUT);
  digitalWrite(EN_OUT_PIN, LOW);  // Default: disabled

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);

  ledcWrite(PWM_PIN, 0);
  digitalWrite(DIR_PIN, HIGH);  // Default: CCW


}

void loop() {
  // ── Enable toggle (GPIO21 external button) ────────────────────────────────
  // Wire an external button between GPIO21 and GND. The board's physical EN/RST
  // button is unrelated and should not be used — it resets the chip in hardware.
  bool enBtn = digitalRead(EN_TOGGLE_PIN);
  if (enBtn == LOW && lastEnBtn == HIGH && (millis() - lastEnTime > DEBOUNCE_MS)) {
    motorEnabled = !motorEnabled;
    lastEnTime = millis();
    digitalWrite(EN_OUT_PIN, motorEnabled ? HIGH : LOW);
    Serial.print("Enable: "); Serial.println(motorEnabled ? "ON" : "OFF");
  }
  lastEnBtn = enBtn;

  // ── Direction toggle (IO0 button) ──────────────────────────────────────────
  bool dirBtn = digitalRead(DIR_TOGGLE_PIN);
  if (dirBtn == LOW && lastDirBtn == HIGH && (millis() - lastDirTime > DEBOUNCE_MS)) {
    dirCCW = !dirCCW;
    digitalWrite(DIR_PIN, dirCCW ? HIGH : LOW);  // HIGH=CCW, LOW=CW
    lastDirTime = millis();
    Serial.print("Direction: "); Serial.println(dirCCW ? "CCW" : "CW");
  }
  lastDirBtn = dirBtn;

  // ── PWM from potentiometer (only when enabled) ─────────────────────────────
  if (motorEnabled) {
    int potRaw  = adcAverage(POT_PIN);
    int pwmDuty = map(potRaw, 0, ADC_FULLSCALE, 0, 255);
    ledcWrite(PWM_PIN, pwmDuty);
  } else {
    ledcWrite(PWM_PIN, 0);
  }

  // ── Feedback readings ──────────────────────────────────────────────────────
  int   currentRaw = adcAverage(ANOUT1_PIN);
  float currentA   = (currentRaw / (float)ADC_FULLSCALE) * MAX_CURRENT;

  int   speedRaw   = adcAverage(ANOUT2_PIN);
  float speedRPM   = (speedRaw / (float)ADC_FULLSCALE) * MAX_RPM;

  Serial.print("En: ");       Serial.print(motorEnabled ? "ON " : "OFF");
  Serial.print(" | Dir: ");   Serial.print(dirCCW ? "CCW" : "CW ");
  Serial.print(" | Speed: "); Serial.print(speedRPM, 0); Serial.print(" RPM");
  Serial.print(" | Current: "); Serial.print(currentA, 3); Serial.println(" A");

  delay(50);
}
