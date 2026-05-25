// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT  6.0f    // 6.0 Amps ESCON output to motor at 3.3V (AnOUT1 full scale)
#define MAX_RPM      10300    // 10300 RPM at 3.3V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

// ── Pin definitions ───────────────────────────────────────────────────────────

#define PWM_PIN_L     27  // PWM speed control output to motor driver
#define EN_OUT_PIN_L  14  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define DIR_PIN_L     32  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define ANOUT1_PIN_L  33  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN_L  25  // AnOUT2: speed feedback ADC input
#define NTC_L         26 //
//to be defined NTC_L

#define PWM_PIN_R     17  // PWM speed control output to motor driver
#define EN_OUT_PIN_R  16  // Enable output to motor driver (HIGH=enabled, LOW=disabled)
#define DIR_PIN_R     5  // Direction output to motor driver (HIGH=CCW, LOW=CW)
#define ANOUT1_PIN_R  15  // AnOUT1: current feedback ADC input
#define ANOUT2_PIN_R  2  // AnOUT2: speed feedback ADC input
#define NTC_R         4 //

// ── Transmission Parameters ────────────────────────────────────────────────────────────────
#define diameter  65    //mm
#define reduction  186

// ── Thermistor Parameters ────────────────────────────────────────────────────────────────
#define T25  298.15
#define Vcc  3.3
#define beta  3490
#define R25  10000 // ohms at 25 degrees C
#define R0  4700 //ohms of fixed resistor in potential divider 

// ── PWM config ────────────────────────────────────────────────────────────────
// Using ESP32 Arduino core v3.x LEDC API (ledcAttach / ledcWrite by pin)
#define PWM_FREQ  20000  // 20 kHz
#define PWM_RES   8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES  16

// ── adcAverage ─────────────────────────────────────────────────────────────────────
// INPUT: pin number
// OUTPUT: digital output average of ADC samples

float adcAverage(int pin) {
  float sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) {
    sum += analogRead(pin); // Note: data for speed and current prefers offset of 100 bits
  }
  return sum / AVG_SAMPLES;
}

// ── feedback ─────────────────────────────────────────────────────────────────────
// INPUT: none
// OUTPUT: none. Computes speed, current and temperature feedback
void feedback() {


    // ── Feedback readings ──────────────────────────────────────────────────────
  float   currentRaw_L = adcAverage(ANOUT1_PIN_L);
  float currentA_L   = (currentRaw_L / (float)ADC_FULLSCALE) * MAX_CURRENT;
  float   currentRaw_R = adcAverage(ANOUT1_PIN_R);
  float currentA_R   = (currentRaw_R / (float)ADC_FULLSCALE) * MAX_CURRENT;

  float   speedRaw_L   = adcAverage(ANOUT2_PIN_L);
  float speedRPM_L   = (speedRaw_L / (float)ADC_FULLSCALE) * MAX_RPM;
  float   speedRaw_R   = adcAverage(ANOUT2_PIN_R);
  float speedRPM_R   = (speedRaw_R / (float)ADC_FULLSCALE) * MAX_RPM;

  float tempRaw_L = adcAverage(NTC_L);
  float V_NTC_L   = (Vcc * tempRaw_L / 4095.0f);
  float R_NTC_L   = R0 * (V_NTC_L / (Vcc - V_NTC_L));
  float T_NTC_L   = (1.0f / (1.0f/T25 + (1.0f/beta) * log(R_NTC_L/R25))) - 273.15f;

  float tempRaw_R = adcAverage(NTC_R);
  float V_NTC_R   = (Vcc * tempRaw_R / 4095.0f);
  float R_NTC_R   = R0 * (V_NTC_R / (Vcc - V_NTC_R));
  float T_NTC_R   = (1.0f / (1.0f/T25 + (1.0f/beta) * log(R_NTC_R/R25))) - 273.15f;



  Serial.print(" | Speed_L: "); Serial.print(speedRPM_L, 0); Serial.print(" RPM");
  Serial.print(" | Speed_R: "); Serial.print(speedRPM_R, 0); Serial.print(" RPM");

  //Serial.print("| Speed_L: "); Serial.print(speedRPM_L*(diameter/2000)/reduction, 0); Serial.print("ms-1");
  //Serial.print("| Speed_R: "); Serial.print(speedRPM_R*(diameter/2000)/reduction, 0); Serial.print("ms-1");

  Serial.println("");
  Serial.print(" | Current_L: "); Serial.print(currentA_L, 3); Serial.println(" A");
  Serial.print(" | Current_R: "); Serial.print(currentA_R, 3); Serial.println(" A");

  Serial.print(" | Temp_L: "); Serial.print(T_NTC_L, 1); Serial.println(" C");
  Serial.print(" | Temp_R: "); Serial.print(T_NTC_R, 1); Serial.println(" C");
  
}





//──Drive Functions  ─────────────────────────────────────────────────────────────────────
// INPUT: speed (0-100)
// OUTPUT: none. PWM is update based on speed and both motors are enabled,
  //to drive forward, backward, turn left or right
void forward(int speed) { 
  digitalWrite(DIR_PIN_L, HIGH); // CCW
  digitalWrite(DIR_PIN_R, LOW);  //  CW

  int pwmDuty = map(speed, 0, 100, 25, 230); //maps 10% and 90% PWM
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}


void backward(int speed) { 
  digitalWrite(DIR_PIN_L, LOW);   //  CW
  digitalWrite(DIR_PIN_R, HIGH);  // CCW

  int pwmDuty = map(speed, 0, 100, 25, 230);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}


void left(int speed) { 
  digitalWrite(DIR_PIN_L, LOW);  // Default: CW
  digitalWrite(DIR_PIN_R, LOW);  // Default: CW

  int pwmDuty = map(speed, 0, 100, 25, 230);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}


void right(int speed) { 
  digitalWrite(DIR_PIN_L, HIGH);  //CCW
  digitalWrite(DIR_PIN_R, HIGH);  //CCW

  int pwmDuty = map(speed, 0, 100, 25, 230);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}

//── Brake  ─────────────────────────────────────────────────────────────────────
// INPUT: none
// OUTPUT: disables the motor. 
void brake() { 
  digitalWrite(EN_OUT_PIN_L, LOW);
  digitalWrite(EN_OUT_PIN_R, LOW);

  int pwmDuty = map(0, 0, 100, 25, 230);
  ledcWrite(PWM_PIN_L, pwmDuty);
  ledcWrite(PWM_PIN_R, pwmDuty);
}


//── Handle Serial  ─────────────────────────────────────────────────────────────────────
// INPUT : none. Detects commands from serial monitor
// OUTPUT: none. Runs drive functions at specified speed
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
      if (value <= 0 && value > 100) value = 70;   // default speed if none given
      forward(value);
      break;

    case 'B':  // Backward
    case 'b':
      if (value <= 0 && value > 100) value = 70;
      backward(value);
      break;

    case 'L':  // Left
    case 'l':
      if (value <= 0 && value > 100) value = 70;
      left(value);
      break;

    case 'R':  // Right
    case 'r':
      if (value <= 0 && value > 100) value = 70;
      right(value);
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

void setup() {
  Serial.begin(115200);
  //pinMode(EN_TOGGLE_PIN,  INPUT_PULLUP);
  //pinMode(DIR_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(NTC_L,    INPUT);
  pinMode(NTC_R,    INPUT);

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

  pinMode(ANOUT1_PIN_R,   INPUT);
  pinMode(ANOUT2_PIN_R,   INPUT);

  pinMode(ANOUT1_PIN_L,   INPUT);
  pinMode(ANOUT2_PIN_L,   INPUT);
}

void loop() {
  
  handleSerial();
  feedback();
  delay(1000);
}