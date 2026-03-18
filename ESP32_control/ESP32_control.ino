// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_RPM      5260    // RPM at 3.3V (AnOUT1 full scale)
#define MAX_CURRENT  1.0f    // Amps at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Pin definitions ───────────────────────────────────────────────────────────
// NOTE: GPIO12 (potentiometer) is temporal — for bench testing only.
//       Long-term, speed setpoint should be received via Bluetooth.
#define POT_PIN     12  // Potentiometer ADC input (0–3.3V → 0–100% PWM)

#define PWM_PIN     19  // PWM speed control output to motor driver
#define DIR_PIN     17  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define ANOUT1_PIN  25  // AnOUT1: speed feedback ADC input
#define ANOUT2_PIN  26  // AnOUT2: current feedback ADC input

// NOTE: GPIO18 (enable toggle) and GPIO0 (direction toggle) are temporal —
//       physical buttons for bench testing only.
#define EN_TOGGLE_PIN  18  // Button input: toggles motor enable on each press
#define DIR_TOGGLE_PIN  0  // IO0 button input: toggles motor direction on each press

// ── PWM config ────────────────────────────────────────────────────────────────
#define PWM_CHANNEL  0
#define PWM_FREQ     20000  // 20 kHz
#define PWM_RES      8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── State ─────────────────────────────────────────────────────────────────────
bool motorEnabled  = false;
bool dirCCW        = true;   // true = CCW (dir1), false = CW (dir2)

bool lastEnBtn  = HIGH;
bool lastDirBtn = HIGH;

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
  pinMode(DIR_PIN, OUTPUT);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);

  ledcWrite(PWM_CHANNEL, 0);
  digitalWrite(DIR_PIN, HIGH);  // Default: CCW
}

void loop() {
  // ── Enable toggle (GPIO18 button) ──────────────────────────────────────────
  bool enBtn = digitalRead(EN_TOGGLE_PIN);
  if (enBtn == LOW && lastEnBtn == HIGH) {
    motorEnabled = !motorEnabled;
    Serial.print("Enable: "); Serial.println(motorEnabled ? "ON" : "OFF");
  }
  lastEnBtn = enBtn;

  // ── Direction toggle (IO0 button) ──────────────────────────────────────────
  bool dirBtn = digitalRead(DIR_TOGGLE_PIN);
  if (dirBtn == LOW && lastDirBtn == HIGH) {
    dirCCW = !dirCCW;
    digitalWrite(DIR_PIN, dirCCW ? HIGH : LOW);  // HIGH=CCW, LOW=CW
    Serial.print("Direction: "); Serial.println(dirCCW ? "CCW" : "CW");
  }
  lastDirBtn = dirBtn;

  // ── PWM from potentiometer (only when enabled) ─────────────────────────────
  if (motorEnabled) {
    int potRaw  = adcAverage(POT_PIN);
    int pwmDuty = map(potRaw, 0, ADC_FULLSCALE, 0, 255);
    ledcWrite(PWM_CHANNEL, pwmDuty);
  } else {
    ledcWrite(PWM_CHANNEL, 0);
  }

  // ── Feedback readings ──────────────────────────────────────────────────────
  int   speedRaw   = adcAverage(ANOUT1_PIN);
  float speedRPM   = (speedRaw / (float)ADC_FULLSCALE) * MAX_RPM;

  int   currentRaw = adcAverage(ANOUT2_PIN);
  float currentA   = (currentRaw / (float)ADC_FULLSCALE) * MAX_CURRENT;

  Serial.print("En: ");       Serial.print(motorEnabled ? "ON " : "OFF");
  Serial.print(" | Dir: ");   Serial.print(dirCCW ? "CCW" : "CW ");
  Serial.print(" | Speed: "); Serial.print(speedRPM, 0); Serial.print(" RPM");
  Serial.print(" | Current: "); Serial.print(currentA, 3); Serial.println(" A");

  delay(50);
}
