// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT   6.0f   // Amps at 3.3V (AnOUT1 full scale)
#define MAX_RPM       10300  // RPM at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Test config ───────────────────────────────────────────────────────────────
#define TEST_DURATION_UP_MS  13000  // ms; change to adjust test length upwards
#define TEST_DURATION_DOWN_MS 10000 //ms; change to adjust test length downwards (to-be manually adjusted in case PID control is imperfect; at the end of the day it's not position control, so inaccuracies may exist)

// ── Pin definitions ───────────────────────────────────────────────────────────
#define PWM_PIN     27  // PWM speed control output to motor driver
#define EN_OUT_PIN  14  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define DIR_PIN     32  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define ANOUT1_PIN  33  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN  25  // AnOUT2: speed feedback ADC input
#define NTC         26  //

// NOTE: GPIO18 avoided — VSPI_CLK, causes reboot when pulled LOW on most boards.
#define EN_TOGGLE_PIN  0  // Button: press to start test (INPUT_PULLUP, active LOW)
#define DIR_TOGGLE_PIN  21  // IO0 button: toggles direction between tests (INPUT_PULLUP, active LOW)

// ── PWM config ────────────────────────────────────────────────────────────────
// Using ESP32 Arduino core v3.x LEDC API (ledcAttach / ledcWrite by pin)
#define PWM_FREQ  20000  // 20 kHz
#define PWM_RES   8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── State ─────────────────────────────────────────────────────────────────────
bool motorEnabled = false;
bool dirCCW       = true;  // true = CCW; overridden by standardTest() during run

#define DIAMETER 24 //mm
#define REDUCTION 186 // 3 stages = 62 * 3

// -- Manually setting PWM Duty
#define PWM_DUTY 40 //theoretical max is 100, 80 works but speed feedback overflows at 10300 somehow, so tried 70, uh 80 seem to work actually
#define DEBOUNCE_MS  50

#define CURRENT_CORRECTION_OFFSET 0.32
#define LINEAR_SPEED_CORRECTION_OFFSET 0.002
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

void toggleDirection() { // not in use
  bool dirBtn = digitalRead(DIR_TOGGLE_PIN);
  if (dirBtn == LOW && lastDirBtn == HIGH && (millis() - lastDirTime > DEBOUNCE_MS)) {
    dirCCW = !dirCCW;
    digitalWrite(DIR_PIN, dirCCW ? HIGH : LOW);
    lastDirTime = millis();
    Serial.print("Direction: "); Serial.println(dirCCW ? "CCW" : "CW");
  }
  lastDirBtn = dirBtn;
}

void forward(int speed) {
  digitalWrite(DIR_PIN, HIGH); 

  int pwmDuty = map(speed, 0, 100, 25, 230); //maps 10% and 90% PWM
  ledcWrite(PWM_PIN, pwmDuty);

  digitalWrite(EN_OUT_PIN, HIGH);
}


void backward(int speed) {
  digitalWrite(DIR_PIN, LOW);  

  int pwmDuty = map(speed, 0, 100, 25, 230); //maps 10% and 90% PWM
  ledcWrite(PWM_PIN, pwmDuty);

  digitalWrite(EN_OUT_PIN, HIGH);
}

void brake() {
  digitalWrite(EN_OUT_PIN, LOW);

  int pwmDuty = map(0, 0, 100, 25, 230);
  ledcWrite(PWM_PIN, pwmDuty);
}

void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd == "ping") { Serial.println("pong"); return; }  // comms check before test
  if (cmd.length() == 0) return;


  char c = cmd.charAt(0);        // first letter: command
  int value = 0;

  // If there are digits after the letter, parse them as speed
  if (cmd.length() > 1) {
    String numPart = cmd.substring(1);
    value = numPart.toInt();     // 0 if not a valid number
  }

  switch (c) {
    case 'F':  // Forward
    case 'f':
      if (value <= 0 || value > 100) value = 70;   // default speed if none given
      forward(value);
      break;

    case 'B':  // Backward
    case 'b':
      if (value <= 0 || value > 100) value = 70;
      backward(value);
      break;

    case 'S':  // Brake
    case 's':
      brake();
      break;

    default:
      Serial.print("Unknown command: ");
      Serial.println(cmd);
      break;
  }
}


// void pwmSend(int pwmDuty) {
//   ledcWrite(PWM_PIN, motorEnabled ? pwmDuty : 0);
// }

void printFeedback() { // for calibration only with life streaming data
    float currentRaw = adcAverage(ANOUT1_PIN);
    float currentA = ((currentRaw / (float)ADC_FULLSCALE) - 0.5f) * 2.0f * MAX_CURRENT + CURRENT_CORRECTION_OFFSET;
    float speedRaw   = adcAverage(ANOUT2_PIN);
    float speedRPM = -((speedRaw / (float)ADC_FULLSCALE) - 0.5f) * 2.0f * MAX_RPM;
  float linearSpeed = speedRPM*((2*3.14/60)*(DIAMETER/2000.0f)/REDUCTION) + LINEAR_SPEED_CORRECTION_OFFSET;

  Serial.print("En: ");         Serial.print(motorEnabled ? "ON " : "OFF");
  Serial.print(" | Speed: ");   Serial.print(speedRPM, 0); Serial.print(" RPM");
  Serial.print(" | Linear Speed: ");   Serial.print(linearSpeed, 0); Serial.print("ms-1");

  Serial.print(" | Current: "); Serial.print(currentA, 3); Serial.println(" A");
}

void standardTest() {
  // int pwmDuty = map(adcAverage(POT_PIN), 0, ADC_FULLSCALE, 25, 230);  // lock setpoint once

  motorEnabled = true;
  digitalWrite(DIR_PIN, LOW);   // CCW fixed for torque test
  digitalWrite(EN_OUT_PIN, HIGH);
  int pwmDuty = map(PWM_DUTY, 0, 100, 25, 230); //maps 10% and 90% PWM
  ledcWrite(PWM_PIN, pwmDuty);

  Serial.println("start");

  unsigned long startTime = millis();
  while (millis() - startTime < TEST_DURATION_UP_MS) {
    unsigned long elapsed = millis() - startTime;
    float currentRaw =  adcAverage(ANOUT1_PIN);
    float currentA = ((currentRaw / (float)ADC_FULLSCALE) - 0.5f) * 2.0f * MAX_CURRENT + CURRENT_CORRECTION_OFFSET;
    float speedRaw   =  adcAverage(ANOUT2_PIN);
    float speedRPM = ((speedRaw / (float)ADC_FULLSCALE) - 0.5f) * 2.0f * MAX_RPM;
    float linearSpeed = speedRPM*((2*3.14/60)*(DIAMETER/2000.0f)/REDUCTION) + LINEAR_SPEED_CORRECTION_OFFSET; 

    Serial.print(elapsed);
    Serial.print(",");
    Serial.print(linearSpeed, 4);
    Serial.print(",");
    Serial.println(currentA, 3);

    delay(50);
  }

  ledcWrite(PWM_PIN, 0);
  // motorEnabled = false;
  digitalWrite(EN_OUT_PIN, LOW);

  Serial.println("stop");
  Serial.flush();

  // wait a bit
  delay(2000); // delay 2000 ms
  // begin moving downwards
  // motorEnabled = true;
  digitalWrite(DIR_PIN, HIGH);   // CW fixed for torque test reset
  digitalWrite(EN_OUT_PIN, HIGH);
  pwmDuty = map(PWM_DUTY, 0, 100, 25, 230); //maps 10% and 90% PWM
  ledcWrite(PWM_PIN, pwmDuty);
  // unsigned long lowerStartTime = millis();
  // while (millis() - lowerStartTime < TEST_DURATION_DOWN_MS) {
  //   delay(50);
  // } // can simplify this, but keep that for now in case i need to add more
  delay(TEST_DURATION_DOWN_MS);

  ledcWrite(PWM_PIN, 0);
  // motorEnabled = false;
  digitalWrite(EN_OUT_PIN, LOW);

  Serial.println("reset_complete");
}

void setup() {
  Serial.begin(115200);

  pinMode(EN_TOGGLE_PIN,  INPUT_PULLUP);
  pinMode(DIR_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(DIR_PIN,    OUTPUT);
  pinMode(EN_OUT_PIN, OUTPUT);
  digitalWrite(EN_OUT_PIN, LOW);   // Default: disabled
  digitalWrite(DIR_PIN,    HIGH);  // Default: CCW

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(PWM_PIN, 0);
}

void loop() {
  handleSerial();
  toggleDirection();

  bool enBtn = digitalRead(EN_TOGGLE_PIN);
  if (enBtn == LOW && lastEnBtn == HIGH && (millis() - lastEnTime > DEBOUNCE_MS)) {
    lastEnTime = millis();
    standardTest();  // blocking: runs full test, then returns
  }
  lastEnBtn = enBtn;
  // printFeedback();
}

