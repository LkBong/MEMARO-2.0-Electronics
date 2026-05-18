// ── Feedback scaling ──────────────────────────────────────────────────────────
#define MAX_CURRENT   1.0f   // Amps at 3.3 V (AnOUT1 full scale)
#define MAX_RPM       5260   // RPM  at 3.3 V (AnOUT2 full scale)
#define ADC_FULLSCALE 4095   // 12-bit ADC max (= 3.3 V)

// ── Pin definitions ───────────────────────────────────────────────────────────
#define PWM_PIN    19   // PWM speed output  (20 kHz, 8-bit)
#define DIR_PIN    17   // Direction output   HIGH=CCW, LOW=CW
#define EN_OUT_PIN 16   // Enable output      HIGH=enabled, LOW=disabled
#define ANOUT1_PIN 25   // AnOUT1: current feedback ADC input
#define ANOUT2_PIN 26   // AnOUT2: speed feedback ADC input

// ── PWM config (ESP32 Arduino core v3.x LEDC API) ────────────────────────────
#define PWM_FREQ 20000  // 20 kHz
#define PWM_RES  8      // 8-bit resolution (0–255)

// ── ADC averaging ─────────────────────────────────────────────────────────────
#define AVG_SAMPLES 16

// ── BLE UUIDs ─────────────────────────────────────────────────────────────────
#define SERVICE_UUID   "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CTRL_CHAR_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"  // Write NR
#define FB_CHAR_UUID   "1C95D5E3-D8F5-4D7F-8B9C-2B7A5C3F1234"  // Notify

#include <NimBLEDevice.h>

// ── State ─────────────────────────────────────────────────────────────────────
static bool motorEnabled = false;
static bool dirCCW       = true;   // true = CCW (GPIO17 HIGH)
static bool bleConnected = false;

static NimBLECharacteristic* pFeedbackChar = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
static void motorStop() {
  ledcWrite(PWM_PIN, 0);
  digitalWrite(EN_OUT_PIN, LOW);
  motorEnabled = false;
}

// ─────────────────────────────────────────────────────────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    // GPIO16 stays LOW; motor stays disabled until first non-zero command
    bleConnected = true;
    Serial.println("BLE connected — awaiting first command");
  }

  void onDisconnect(NimBLEServer*) override {
    motorStop();   // PWM → 0, GPIO16 LOW, motorEnabled = false
    bleConnected = false;
    Serial.println("BLE disconnected — motor stopped");
    NimBLEDevice::startAdvertising();
  }
};

// ─────────────────────────────────────────────────────────────────────────────
class ControlCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) override {
    NimBLEAttValue val = pChar->getValue();
    if (val.length() < 3) return;

    int8_t  x     = (int8_t)val[0];   // left/right  –100…100
    int8_t  y     = (int8_t)val[1];   // fwd/back    –100…100
    uint8_t speed = val[2];            // PWM duty    0…255

    // TODO: differential drive — map x/y to independent left/right motor
    //       speeds once second motor is wired (x currently unused).
    (void)x;

    // Direction from y; no change when y == 0 (speed will be 0 anyway)
    if (y > 0) {
      dirCCW = false;
      digitalWrite(DIR_PIN, LOW);   // CW  = forward
    } else if (y < 0) {
      dirCCW = true;
      digitalWrite(DIR_PIN, HIGH);  // CCW = backward
    }

    // Enable motor on first non-zero speed command
    if (speed > 0 && !motorEnabled) {
      digitalWrite(EN_OUT_PIN, HIGH);
      motorEnabled = true;
    }

    ledcWrite(PWM_PIN, speed);

    // Disable if commanded to stop (keeps GPIO16 LOW until next non-zero command)
    if (speed == 0 && motorEnabled) {
      digitalWrite(EN_OUT_PIN, LOW);
      motorEnabled = false;
    }
  }
};

// ─────────────────────────────────────────────────────────────────────────────
static int adcAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) sum += analogRead(pin);
  return (int)(sum / AVG_SAMPLES);
}

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN,    OUTPUT);
  pinMode(EN_OUT_PIN, OUTPUT);
  digitalWrite(EN_OUT_PIN, LOW);   // disabled until first command
  digitalWrite(DIR_PIN,    HIGH);  // default: CCW

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(PWM_PIN, 0);

  NimBLEDevice::init("MEMARO");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  NimBLECharacteristic* pCtrlChar = pService->createCharacteristic(
    CTRL_CHAR_UUID, NIMBLE_PROPERTY::WRITE_NR);
  pCtrlChar->setCallbacks(new ControlCallbacks());

  pFeedbackChar = pService->createCharacteristic(
    FB_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

  pService->start();
  NimBLEDevice::startAdvertising();
  Serial.println("BLE advertising as MEMARO");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  int   currentRaw = adcAverage(ANOUT1_PIN);
  float currentA   = (currentRaw / (float)ADC_FULLSCALE) * MAX_CURRENT;

  int   speedRaw   = adcAverage(ANOUT2_PIN);
  float speedRPM   = (speedRaw  / (float)ADC_FULLSCALE) * MAX_RPM;

  if (bleConnected && pFeedbackChar != nullptr) {
    uint16_t rpmInt    = (uint16_t)speedRPM;
    uint16_t currentMA = (uint16_t)(currentA * 1000.0f);

    uint8_t payload[4] = {
      (uint8_t)(rpmInt    >> 8), (uint8_t)(rpmInt    & 0xFF),
      (uint8_t)(currentMA >> 8), (uint8_t)(currentMA & 0xFF)
    };
    pFeedbackChar->setValue(payload, 4);
    pFeedbackChar->notify();
  }

  Serial.print("BLE: "); Serial.print(bleConnected ? "CONN" : "ADV ");
  Serial.print(" | En: ");      Serial.print(motorEnabled ? "ON " : "OFF");
  Serial.print(" | Dir: ");     Serial.print(dirCCW ? "CCW" : "CW ");
  Serial.print(" | Speed: ");   Serial.print(speedRPM, 0); Serial.print(" RPM");
  Serial.print(" | Current: "); Serial.print(currentA, 3); Serial.println(" A");

  delay(50);
}
