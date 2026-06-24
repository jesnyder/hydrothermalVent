/*
  Date: 2026-02-17 (updated 2026-06-16)
  Objective: Control a NEMA 17 stepper motor with variable speed, direction,
  and endstop safety via Serial Monitor. Optionally reads temperature and
  humidity from an SHT30 sensor and controls a heating pad.

  Wiring — Stepper:
    - Arduino Pin 7 -> DM542 PUL-
    - Arduino Pin 6 -> DM542 DIR-
    - PUL+ and DIR+ -> Arduino 5V
    - ENA disconnected or ENA- tied LOW (GND)
    - DM542 A+/A-, B+/B- -> stepper motor coils
        A+ : Black
        A- : Green
        B+ : Red
        B- : Blue
    - DM542 V+/V- -> 24V DC power supply
    - Limit Switch (Endstop):
        RED wire  - COM -> Arduino GND
        BLACK wire - NC -> Arduino Pin 9
    - DIP switches: SW1=ON, SW2=OFF, SW3=ON (2A);
                    SW4=ON, SW5=OFF, SW6=OFF, SW7=OFF; SW8=ON

  Wiring — SHT30 sensor (optional):
    - SDA -> Arduino SDA (Pin 20 on Mega)
    - SCL -> Arduino SCL (Pin 21 on Mega)
    - VCC -> 3.3V or 5V
    - GND -> GND

  Wiring — Heating pad (optional):
    - MOSFET Gate   -> Arduino Pin 8
    - MOSFET Drain  -> Heater negative terminal
    - Heater positive terminal -> supply +
    - MOSFET Source -> GND (common with Arduino)
    - Flyback diode across heater terminals if inductive load

  Serial commands:
    MXX        -> Set motor speed 0-100%
    F          -> Forward direction
    R          -> Reverse direction
    H <temp>   -> Enable heater and set target temperature in °C (1-80)
    H OFF      -> Disable heater
*/

#include <Wire.h>
#include <Adafruit_SHT31.h>

// ----------------------- Stepper Pins --------------------------
const int stepPin    = 7;
const int dirPin     = 6;
const int endstopPin = 9;

// ----------------------- Heater Pin ----------------------------
const int heaterPin = 8;

// ----------------------- Motor State ---------------------------
float motorSpeedPercent = 0;
bool  directionForward  = true;
float currentDelay      = 50000; // start at maxDelay (motor stopped)
float targetDelay       = 50000;

const float minDelay  = 1000;  // fastest speed (us between steps)
const float maxDelay  = 50000; // slowest speed (us between steps)
const float accelStep = 5;     // us per loop iteration for ramping

unsigned long stepCount     = 0;
unsigned long lastStepTime  = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval  = 1000; // status print every 1s

// ----------------------- SHT30 Sensor -------------------------
Adafruit_SHT31 sht30 = Adafruit_SHT31();
bool  sht30Available = false;
float temperature    = NAN;
float humidity       = NAN;

unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 5000; // read sensor every 5s

// ----------------------- Heater Control -----------------------
float heaterTargetTemp    = 40.0;
float heaterTempTolerance = 1.0;
bool  heaterEnabled       = false; // off by default; enable with "H <temp>"

void setup() {
  Serial.begin(9600);

  pinMode(stepPin,    OUTPUT);
  pinMode(dirPin,     OUTPUT);
  pinMode(heaterPin,  OUTPUT);
  pinMode(endstopPin, INPUT_PULLUP);

  digitalWrite(dirPin, HIGH);
  analogWrite(heaterPin, 0);

  Wire.begin();
  if (sht30.begin(0x44)) {
    sht30Available = true;
    Serial.println("SHT30 detected — temperature logging and heater available.");
  } else {
    sht30Available = false;
    Serial.println("SHT30 not found — temperature logging and heater disabled.");
  }

  Serial.println("Stepper Motor Controller Initialized.");
  Serial.println("Commands:");
  Serial.println("  MXX       -> Set speed 0-100%");
  Serial.println("  F         -> Forward direction");
  Serial.println("  R         -> Reverse direction");
  Serial.println("  H <temp>  -> Enable heater, set target deg C (1-80)");
  Serial.println("  H OFF     -> Disable heater");
  Serial.println("Motor starts OFF.");
}

void loop() {
  // --- Serial command handling ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Keep a copy for case-sensitive value extraction (e.g. H 39.5)
    String inputUpper = input;
    inputUpper.toUpperCase();

    if (inputUpper.startsWith("M")) {
      float speed = constrain(input.substring(1).toFloat(), 0, 100);
      motorSpeedPercent = speed;
      targetDelay = (motorSpeedPercent == 0)
        ? maxDelay
        : maxDelay - (motorSpeedPercent / 100.0) * (maxDelay - minDelay);
      Serial.print("Speed set to "); Serial.print(motorSpeedPercent); Serial.println("%");

    } else if (inputUpper == "F") {
      directionForward = true;
      digitalWrite(dirPin, HIGH);
      Serial.println("Direction: Forward");

    } else if (inputUpper == "R") {
      directionForward = false;
      digitalWrite(dirPin, LOW);
      Serial.println("Direction: Reverse");

    } else if (inputUpper.startsWith("H")) {
      String arg = input.substring(1);
      arg.trim();
      String argUpper = arg;
      argUpper.toUpperCase();

      if (argUpper == "OFF") {
        heaterEnabled = false;
        analogWrite(heaterPin, 0);
        Serial.println("Heater disabled.");
      } else if (!sht30Available) {
        Serial.println("Cannot enable heater: SHT30 not detected.");
      } else {
        float temp = arg.toFloat();
        if (temp < 1 || temp > 80) {
          Serial.println("Invalid temperature. Use 1-80 deg C.");
        } else {
          heaterTargetTemp = temp;
          heaterEnabled    = true;
          Serial.print("Heater enabled, target: ");
          Serial.print(heaterTargetTemp, 1);
          Serial.println(" deg C");
        }
      }

    } else {
      Serial.println("Unknown command. Use MXX, F, R, H <temp>, or H OFF.");
    }
  }

  // --- Endstop check ---
  if (digitalRead(endstopPin) == LOW) {
    motorSpeedPercent = 0;
    targetDelay       = maxDelay;
    Serial.println("Endstop triggered! Motor stopped.");
    directionForward = !directionForward;
    digitalWrite(dirPin, directionForward ? HIGH : LOW);
    delay(200);
  }

  // --- Smooth acceleration/deceleration ---
  if (currentDelay < targetDelay) {
    currentDelay += accelStep;
    if (currentDelay > targetDelay) currentDelay = targetDelay;
  } else if (currentDelay > targetDelay) {
    currentDelay -= accelStep;
    if (currentDelay < targetDelay) currentDelay = targetDelay;
  }

  // --- Step motor ---
  if (motorSpeedPercent > 0) {
    unsigned long now = micros();
    if (now - lastStepTime >= (unsigned long)currentDelay) {
      lastStepTime = now;
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(5);
      digitalWrite(stepPin, LOW);
      stepCount++;
    }
  }

  // --- Read SHT30 every 5s ---
  if (sht30Available && millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    temperature = sht30.readTemperature();
    humidity    = sht30.readHumidity();
  }

  // --- Heater control (fail-safe: turns off on sensor error) ---
  if (heaterEnabled) {
    if (!sht30Available || isnan(temperature)) {
      analogWrite(heaterPin, 0);
    } else if (temperature < heaterTargetTemp - heaterTempTolerance) {
      analogWrite(heaterPin, 255);
    } else if (temperature > heaterTargetTemp + heaterTempTolerance) {
      analogWrite(heaterPin, 0);
    }
  }

  // --- Print status every 1s ---
  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();

    Serial.print("Speed(%): ");   Serial.print(motorSpeedPercent);
    Serial.print(" | Delay(us): "); Serial.print(currentDelay);
    Serial.print(" | Steps: ");   Serial.print(stepCount);

    if (sht30Available) {
      Serial.print(" | Temp(C): ");
      if (isnan(temperature)) Serial.print("err");
      else Serial.print(temperature, 2);

      Serial.print(" | Humidity(%): ");
      if (isnan(humidity)) Serial.print("err");
      else Serial.print(humidity, 2);
    }

    if (heaterEnabled) {
      Serial.print(" | Heater target(C): ");
      Serial.print(heaterTargetTemp, 1);
    }

    Serial.println();
  }
}
