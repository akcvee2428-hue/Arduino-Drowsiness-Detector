#include <Wire.h>

const int MPU_ADDR = 0x68; // MPU6050 I2C Address

// Pin Definitions
const int GREEN_LED = 2; // Normal condition indicator (Pin D2)
const int RED_LED   = 3; // Drowsiness alert LED (Pin D3)
const int BUZZER    = 5; // Alert Buzzer (Pin D5)

// Sensitivity & Delay Settings
const float TILT_THRESHOLD = 25.0;            // Tilt trigger angle (degrees)
const unsigned long DROWSY_TIME_LIMIT = 1500; // Continuous tilt required to trigger (1.5 seconds)

unsigned long drowsyStartTime = 0;
bool isDrowsyTimerRunning = false;

// Sensor calibration offsets
float accelX_offset = 0, accelY_offset = 0, accelZ_offset = 0;

void readRawAccel(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() >= 6) {
    x = (Wire.read() << 8) | Wire.read();
    y = (Wire.read() << 8) | Wire.read();
    z = (Wire.read() << 8) | Wire.read();
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Configure output pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Initial State: Normal (Green ON, Red OFF, Buzzer OFF)
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  // Wake up MPU6050 sensor
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println(F("Calibrating MPU6050... Keep sensor LEVEL and STILL."));
  delay(1000);

  // Auto-zero calibration routine
  int16_t rawX, rawY, rawZ;
  long sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < 50; i++) {
    readRawAccel(rawX, rawY, rawZ);
    sumX += rawX;
    sumY += rawY;
    sumZ += rawZ;
    delay(10);
  }
  accelX_offset = sumX / 50.0;
  accelY_offset = sumY / 50.0;
  accelZ_offset = (sumZ / 50.0) - 16384.0;

  Serial.println(F("Calibration Complete! Drowsiness Detection System Active."));
}

void loop() {
  int16_t rawX, rawY, rawZ;
  readRawAccel(rawX, rawY, rawZ);

  // Convert raw readings to G-force
  float ax = (rawX - accelX_offset) / 16384.0;
  float ay = (rawY - accelY_offset) / 16384.0;
  float az = (rawZ - accelZ_offset) / 16384.0;

  // Calculate tilt angles (Pitch & Roll)
  float pitch = abs(atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI);
  float roll  = abs(atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI);

  // Output real-time values to Serial Monitor
  Serial.print(F("Pitch: "));
  Serial.print(pitch, 1);
  Serial.print(F("° | Roll: "));
  Serial.print(roll, 1);
  Serial.println(F("°"));

  // Check if head tilt exceeds threshold
  if (pitch > TILT_THRESHOLD || roll > TILT_THRESHOLD) {
    if (!isDrowsyTimerRunning) {
      drowsyStartTime = millis();
      isDrowsyTimerRunning = true;
    } 
    else if (millis() - drowsyStartTime >= DROWSY_TIME_LIMIT) {
      // DROWSINESS ALERT: Green OFF, Red ON, Buzzer ON
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BUZZER, HIGH);
      Serial.println(F(">>> DROWSINESS ALERT DETECTED! <<<"));
    }
  } else {
    // NORMAL STATE: Green ON, Red OFF, Buzzer OFF
    isDrowsyTimerRunning = false;
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }

  delay(100);
}
