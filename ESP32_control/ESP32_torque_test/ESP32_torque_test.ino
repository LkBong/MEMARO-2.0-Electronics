// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT   1.0f   // Amps at 3.3V (AnOUT1 full scale)
#define MAX_RPM       10300  // RPM at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Test config ───────────────────────────────────────────────────────────────
#define TEST_DURATION_MS  10000  // ms; change to adjust test length

// ── Pin definitions ───────────────────────────────────────────────────────────
#define POT_PIN        4   // Potentiometer ADC input — locks PWM setpoint at test start
#define PWM_PIN       19   // PWM speed control output to motor driver
#define DIR_PIN       17   // Direction output (HIGH=CCW, LOW=CW)
#define EN_OUT_PIN    16   // Enable output (HIGH=enabled, LOW=disabled)
#define ANOUT1_PIN    25   // AnOUT1: current feedback ADC input
#define ANOUT2_PIN    26   // AnOUT2: speed feedback ADC input

// NOTE: GPIO18 avoided — VSPI_CLK, causes reboot when pulled LOW on most boards.
#define EN_TOGGLE_PIN  21  // Button: press to start test (INPUT_PULLUP, active LOW)

// ── PWM config ────────────────────────────────────────────────────────────────
// Using ESP32 Arduino core v3.x LEDC API (ledcAttach / ledcWrite by pin)
#define PWM_FREQ  20000  // 20 kHz
#define PWM_RES   8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── State ─────────────────────────────────────────────────────────────────────
bool motorEnabled = false;

#define DEBOUNCE_MS  50

bool lastEnBtn = HIGH;
unsigned long lastEnTime = 0;

// ─────────────────────────────────────────────────────────────────────────────
int adcAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return sum / AVG_SAMPLES;
}

void toggleEnable() {
  bool enBtn = digitalRead(EN_TOGGLE_PIN);
  if (enBtn == LOW && lastEnBtn == HIGH && (millis() - lastEnTime > DEBOUNCE_MS)) {
    motorEnabled = !motorEnabled;
    lastEnTime = millis();
    digitalWrite(EN_OUT_PIN, motorEnabled ? HIGH : LOW);
    Serial.print("Enable: "); Serial.println(motorEnabled ? "ON" : "OFF");
  }
  lastEnBtn = enBtn;
}

void toggleDirection() {
  // Reserved for manual operation mode — not used in standardTest()
}

void pwmSend(int pwmDuty) {
  ledcWrite(PWM_PIN, motorEnabled ? pwmDuty : 0);
}

void printFeedback() {
  float currentA = (adcAverage(ANOUT1_PIN) / (float)ADC_FULLSCALE) * MAX_CURRENT;
  float speedRPM = (adcAverage(ANOUT2_PIN) / (float)ADC_FULLSCALE) * MAX_RPM;

  Serial.print("En: ");         Serial.print(motorEnabled ? "ON " : "OFF");
  Serial.print(" | Speed: ");   Serial.print(speedRPM, 0); Serial.print(" RPM");
  Serial.print(" | Current: "); Serial.print(currentA, 3); Serial.println(" A");
}

void standardTest() {
  int pwmDuty = map(adcAverage(POT_PIN), 0, ADC_FULLSCALE, 25, 179);  // lock setpoint once

  motorEnabled = true;
  digitalWrite(EN_OUT_PIN, HIGH);
  digitalWrite(DIR_PIN, HIGH);   // CCW fixed for torque test
  ledcWrite(PWM_PIN, pwmDuty);

  Serial.println("start");

  unsigned long startTime = millis();
  while (millis() - startTime < TEST_DURATION_MS) {
    unsigned long elapsed = millis() - startTime;
    float currentA = (adcAverage(ANOUT1_PIN) / (float)ADC_FULLSCALE) * MAX_CURRENT;
    float speedRPM = (adcAverage(ANOUT2_PIN) / (float)ADC_FULLSCALE) * MAX_RPM;

    Serial.print(elapsed);
    Serial.print(",");
    Serial.print(speedRPM, 0);
    Serial.print(",");
    Serial.println(currentA, 3);

    delay(50);
  }

  ledcWrite(PWM_PIN, 0);
  motorEnabled = false;
  digitalWrite(EN_OUT_PIN, LOW);

  Serial.println("stop");
  Serial.flush();
}

void setup() {
  Serial.begin(115200);

  pinMode(EN_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(DIR_PIN,    OUTPUT);
  pinMode(EN_OUT_PIN, OUTPUT);
  digitalWrite(EN_OUT_PIN, LOW);   // Default: disabled
  digitalWrite(DIR_PIN,    HIGH);  // Default: CCW

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(PWM_PIN, 0);
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "ping") Serial.println("pong");  // comms check before test
  }

  bool enBtn = digitalRead(EN_TOGGLE_PIN);
  if (enBtn == LOW && lastEnBtn == HIGH && (millis() - lastEnTime > DEBOUNCE_MS)) {
    lastEnTime = millis();
    standardTest();  // blocking: runs full test, then returns
  }
  lastEnBtn = enBtn;
}
