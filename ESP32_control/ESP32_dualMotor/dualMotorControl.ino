

// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT  1.0f    // Amps at 3.3V (AnOUT1 full scale)
#define MAX_RPM      10300    // RPM at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Pin definitions ───────────────────────────────────────────────────────────
// NOTE: GPIO12 (potentiometer) is temporal — for bench testing only.
//       Long-term, speed setpoint should be received via Bluetooth.
//#define POT_PIN      4  // Potentiometer ADC input (0–3.3V → 0–100% PWM)

#define PWM_PIN_L     19  // PWM speed control output to motor driver
#define DIR_PIN_L     17  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define EN_OUT_PIN_L  16  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define ANOUT1_PIN_L  25  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN_L  26  // AnOUT2: speed feedback ADC input
//to be defined NTC_L

#define PWM_PIN_R     23  // PWM speed control output to motor driver
#define DIR_PIN_R     21  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define EN_OUT_PIN_R  22  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define ANOUT1_PIN_R  36  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN_R  39  // AnOUT2: speed feedback ADC input
//to be defined NTC_R

// NOTE: GPIO21 (enable toggle) and GPIO0 (direction toggle) are temporal —
//       physical buttons for bench testing only.
//       GPIO18 avoided — VSPI_CLK, causes reboot when pulled LOW on most boards.
// #define EN_TOGGLE_PIN  21  // Button input: toggles motor enable on each press
// #define DIR_TOGGLE_PIN  0  // IO0 button input: toggles motor direction on each press

// ── PWM config ────────────────────────────────────────────────────────────────
// Using ESP32 Arduino core v3.x LEDC API (ledcAttach / ledcWrite by pin)
#define PWM_FREQ  20000  // 20 kHz
#define PWM_RES   8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── State ─────────────────────────────────────────────────────────────────────
// bool motorEnabled  = false;
// bool dirCCW        = true;   // true = CCW (dir1), false = CW (dir2)

// #define DEBOUNCE_MS  50  // ms to wait before registering a button press

// bool lastEnBtn  = HIGH;
// bool lastDirBtn = HIGH;

// unsigned long lastEnTime  = 0;
// unsigned long lastDirTime = 0; 

// ─────────────────────────────────────────────────────────────────────────────
int adcAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return sum / AVG_SAMPLES;
}

//forward function: input of PWM out of 90% and drive both motors at the specified speed
    //to propel the rover forward
void forward(int speed) { 
  digitalWrite(DIR_PIN_L, HIGH);  // CCW
  digitalWrite(DIR_PIN_R, LOW);  //  CW

  int pwmDuty = map(speed, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}

//backward function: input of PWM out of 90% and drive both motors at the specified speed
    //to propel the rover backward
void backward(int speed) { 
  digitalWrite(DIR_PIN_L, LOW);  //  CW
  digitalWrite(DIR_PIN_R, HIGH);  // CCW

  int pwmDuty = map(speed, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}

//left function: input of PWM out of 90% and drive both motors at the specified speed
    //to rotate the rover left
void left(int speed) { 
  digitalWrite(DIR_PIN_L, LOW);  // Default: CW
  digitalWrite(DIR_PIN_R, LOW);  // Default: CW

  int pwmDuty = map(speed, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}

//left function: input of PWM out of 90% and drive both motors at the specified speed
    //to rotate the rover right
void right(int speed) { 
  digitalWrite(DIR_PIN_L, HIGH);  //CCW
  digitalWrite(DIR_PIN_R, HIGH);  //CCW

  int pwmDuty = map(speed, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}

void brake() { 
  digitalWrite(EN_OUT_PIN_L, LOW);
  digitalWrite(EN_OUT_PIN_R, LOW);

  int pwmDuty = map(0, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);
}


void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n'); // read line up to newline
  cmd.trim();                                // remove spaces, CR, etc.
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
      if (value <= 0) value = 200;   // default speed if none given
      forward(value);
      Serial.print("Forward, speed=");
      Serial.println(value);
      break;

    case 'B':  // Backward
    case 'b':
      if (value <= 0) value = 200;
      backward(value);
      Serial.print("Backward, speed=");
      Serial.println(value);
      break;

    case 'L':  // Left
    case 'l':
      if (value <= 0) value = 200;
      left(value);
      Serial.print("Left, speed=");
      Serial.println(value);
      break;

    case 'R':  // Right
    case 'r':
      if (value <= 0) value = 200;
      right(value);
      Serial.print("Right, speed=");
      Serial.println(value);
      break;

    case 'S':  // Brake
    case 's':
      brake();
      Serial.println("Brake");
      break;


    default:
      Serial.print("Unknown command: ");
      Serial.println(cmd);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  //pinMode(EN_TOGGLE_PIN,  INPUT_PULLUP);
  //pinMode(DIR_TOGGLE_PIN, INPUT_PULLUP);


  pinMode(DIR_PIN_L,    OUTPUT);
  pinMode(EN_OUT_PIN_L, OUTPUT);
  digitalWrite(EN_OUT_PIN_L, LOW);  // Default: disabled

  pinMode(DIR_PIN_R,    OUTPUT);
  pinMode(EN_OUT_PIN_R, OUTPUT);
  digitalWrite(EN_OUT_PIN_R, LOW);  // Default: disabled

  ledcAttach(PWM_PIN_L, PWM_FREQ, PWM_RES);
  ledcAttach(PWM_PIN_R, PWM_FREQ, PWM_RES);

  ledcWrite(PWM_PIN_L, 0);
  digitalWrite(DIR_PIN_L, HIGH);  // CCW

  ledcWrite(PWM_PIN_R, 0);
  digitalWrite(DIR_PIN_R, LOW);  // Default: CW
}

void loop() {
  
  handleSerial();

  // ── Feedback readings ──────────────────────────────────────────────────────
  int   currentRaw_L = adcAverage(ANOUT1_PIN_L);
  float currentA_L   = (currentRaw_L / (float)ADC_FULLSCALE) * MAX_CURRENT;
  int   currentRaw_R = adcAverage(ANOUT1_PIN_R);
  float currentA_R   = (currentRaw_R / (float)ADC_FULLSCALE) * MAX_CURRENT;

  int   speedRaw_L   = adcAverage(ANOUT2_PIN_L);
  float speedRPM_L   = (speedRaw_L / (float)ADC_FULLSCALE) * MAX_RPM;
  int   speedRaw_R   = adcAverage(ANOUT2_PIN_R);
  float speedRPM_R   = (speedRaw_R / (float)ADC_FULLSCALE) * MAX_RPM;


  Serial.print(" | Speed_L: "); Serial.print(speedRPM_L, 0); Serial.print(" RPM");
  Serial.print(" | Speed_R: "); Serial.print(speedRPM_R, 0); Serial.print(" RPM");

  Serial.print(" | Current_L: "); Serial.print(currentA_L, 3); Serial.println(" A");
  Serial.print(" | Current_R: "); Serial.print(currentA_R, 3); Serial.println(" A");

  delay(50);
}
