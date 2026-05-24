// ── Feedback scaling definitions ─────────────────────────────────────────────
#define MAX_CURRENT  1.0f    // Amps at 3.3V (AnOUT1 full scale)
#define MAX_RPM      10300    // RPM at 3.3V (AnOUT2 full scale)
//#define ADC_FULLSCALE 4095   // 12-bit ADC max count (= 3.3V)

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
#define DIR_PIN_R     1  // Direction output to motor driver (HIGH=CCW, LOW=CW)
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

void setup() {
  Serial.begin(115200);

//   pinMode(EN_TOGGLE_PIN,  INPUT_PULLUP);
//   pinMode(DIR_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(DIR_PIN_L,    OUTPUT);
  pinMode(EN_OUT_PIN_L, OUTPUT);
  digitalWrite(EN_OUT_PIN_L, LOW);  // Default: disabled

  pinMode(DIR_PIN_R,    OUTPUT);
  pinMode(EN_OUT_PIN_R, OUTPUT);
  digitalWrite(EN_OUT_PIN_R, LOW);  // Default: disabled

  ledcAttach(PWM_PIN_L, PWM_FREQ, PWM_RES);
  ledcAttach(PWM_PIN_R, PWM_FREQ, PWM_RES);

  ledcWrite(PWM_PIN_L, 0);
  digitalWrite(DIR_PIN_L, HIGH);  // Default: CCW

  ledcWrite(PWM_PIN_R, 0);
  digitalWrite(DIR_PIN_R, LOW);  // Default: CW

}

//forward fu input of PWM out of 90% and drive both motors at the specified speed
void forward(int speed) { 
  digitalWrite(DIR_PIN_L, HIGH);  // Default: CCW
  digitalWrite(DIR_PIN_R, LOW);  // Default: CW

  int pwmDuty = map(speed, 0, 100, 25, 179);
  ledcWrite(PWM_PIN_L), pwmDuty);
  ledcWrite(PWM_PIN_R), pwmDuty);

  digitalWrite(EN_OUT_PIN_L, HIGH);
  digitalWrite(EN_OUT_PIN_R, HIGH);
}



void loop() {
  

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