/***************************************************************
 * Date: 2026-01-27
 * Objective: Independently control two peristaltic pumps, log
 * system status to Serial (time, pump speeds, temperature,
 * humidity), and allow operation even if the SHT30 sensor fails.
 *
 * Libraries required:
 * - Wire.h        (built-in)
 * - Adafruit_SHT31 (install via Arduino Library Manager)
 *
 * Hardware Setup:
 * -------------------------
 * Pump 1:
 *   - MOSFET drain -> Pump 1 negative terminal
 *   - Pump 1 positive terminal -> 6V supply +
 *   - MOSFET gate -> Arduino pin 9
 *   - MOSFET source -> Arduino GND
 *   - 1N4007 diode across pump terminals (cathode to +)
 *
 * Pump 2:
 *   - MOSFET drain -> Pump 2 negative terminal
 *   - Pump 2 positive terminal -> 6V supply +
 *   - MOSFET gate -> Arduino pin 8
 *   - MOSFET source -> Arduino GND
 *   - 1N4007 diode across pump terminals (cathode to +)
 *
 * SHT30 Sensor:
 *   - VCC (Red)   -> 3.3V or 5V
 *   - GND (Black) -> GND
 *   - SCL (Yellow)-> Arduino Mega Pin 21
 *   - SDA (White) -> Arduino Mega Pin 20
 ***************************************************************/

#include <Wire.h>
#include <Adafruit_SHT31.h>

// ----------------------- Pin Definitions -----------------------
const int pump1Pin = 9;   // Pump 1 MOSFET gate
const int pump2Pin = 8;   // Pump 2 MOSFET gate

// ----------------------- PWM Settings -------------------------
const int maxPWM = 255; // max speed
const int minPWM = 50;  // minimum PWM to avoid pump stalling

// ----------------------- Pump States --------------------------
int pump1SpeedPWM = 0;  // Pump 1 starts OFF
int pump2SpeedPWM = 0;  // Pump 2 starts OFF

// ----------------------- SHT30 Sensor -------------------------
Adafruit_SHT31 sht30 = Adafruit_SHT31();
bool sht30Available = false;

// ----------------------- Time Tracking ------------------------
unsigned long startMillis;

// ----------------------- Setup Function -----------------------
void setup() {
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);

  analogWrite(pump1Pin, pump1SpeedPWM);
  analogWrite(pump2Pin, pump2SpeedPWM);

  Serial.begin(9600);
  Serial.println("Two-Pump Control Initialized.");

  startMillis = millis();

  // Initialize SHT30 sensor
  if (sht30.begin(0x44)) { // Default I2C address
    sht30Available = true;
    Serial.println("SHT30 sensor detected.");
  } else {
    Serial.println("SHT30 - not found. Pumps will still operate.");
    sht30Available = false;
  }

  // Print header for CSV-style output
  Serial.println("Time(HH:MM),Pump1_Speed%,Pump2_Speed%,Temp_C,Humidity_%");
  Serial.println("Commands: 1 <0-100>, 2 <0-100>, B <0-100>");
}

// ----------------------- Main Loop ----------------------------
void loop() {
  // ----------------- Serial Input Handling -----------------
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      int spaceIndex = input.indexOf(' ');
      if (spaceIndex > 0) {
        String pumpStr = input.substring(0, spaceIndex);
        String speedStr = input.substring(spaceIndex + 1);
        speedStr.trim();

        int speedPercent = speedStr.toInt();
        if (speedPercent < 0) speedPercent = 0;
        if (speedPercent > 100) speedPercent = 100;

        // Map 0-100% to PWM (0-255)
        int pwmValue = map(speedPercent, 0, 100, 0, maxPWM);
        if (pwmValue > 0 && pwmValue < minPWM) pwmValue = minPWM;

        // CONTROL PUMPS REGARDLESS OF SENSOR STATUS
        if (pumpStr == "1") {
          pump1SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          Serial.print("Pump 1 on Pin 9 - Speed: "); Serial.print(speedPercent); Serial.println("%");
        }
        else if (pumpStr == "2") {
          pump2SpeedPWM = pwmValue;
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Pump 2 on Pin 8 - Speed: "); Serial.print(speedPercent); Serial.println("%");
        }
        else if (pumpStr == "B" || pumpStr == "b") {
          pump1SpeedPWM = pwmValue;
          pump2SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Both pumps set to: "); Serial.print(speedPercent); Serial.println("%");
        }
        else {
          Serial.println("Invalid pump identifier. Use 1, 2, or B.");
        }
      }
      else {
        Serial.println("Invalid input format. Example: '1 50'");
      }
    }

    // Clear any leftover input
    while (Serial.available() > 0) Serial.read();
  }

  // ----------------- Read SHT30 Sensor ---------------------
  float temperature = NAN;
  float humidity = NAN;

  if (sht30Available) {
    temperature = sht30.readTemperature();
    humidity = sht30.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Warning: Failed to read from SHT30. Logging pumps only.");
    }
  }

  // ----------------- Calculate Elapsed Time -----------------
  unsigned long elapsedSeconds = (millis() - startMillis) / 1000;
  unsigned long hours = elapsedSeconds / 3600;
  unsigned long minutes = (elapsedSeconds % 3600) / 60;

  // Convert PWM back to % for logging
  int pump1Percent = map(pump1SpeedPWM, 0, maxPWM, 0, 100);
  int pump2Percent = map(pump2SpeedPWM, 0, maxPWM, 0, 100);

  // ----------------- Print Status to Serial -----------------
  Serial.print(hours); Serial.print(":");
  if (minutes < 10) Serial.print("0"); // pad single digit
  Serial.print(minutes); Serial.print(",");
  Serial.print(pump1Percent); Serial.print(",");
  Serial.print(pump2Percent); Serial.print(",");
  if (!isnan(temperature)) Serial.print(temperature, 2);
  Serial.print(",");
  if (!isnan(humidity)) Serial.print(humidity, 2);
  Serial.println("");

  delay(5000); // log every 5 seconds
}


  // ----------------- Calculate Elapsed Time -----------------
  unsigned long elapsedSeconds = (millis() - startMillis) / 1000;
  unsigned long hours = elapsedSeconds / 3600;
  unsigned long minutes = (elapsedSeconds % 3600) / 60;

  // Convert PWM back to % for logging
  int pump1Percent = map(pump1SpeedPWM, 0, maxPWM, 0, 100);
  int pump2Percent = map(pump2SpeedPWM, 0, maxPWM, 0, 100);

  // ----------------- Print Status to Serial -----------------
  Serial.print(hours); Serial.print(":");
  if (minutes < 10) Serial.print("0"); // pad single digit
  Serial.print(minutes); Serial.print(",");
  Serial.print(pump1Percent); Serial.print(",");
  Serial.print(pump2Percent); Serial.print(",");
  if (!isnan(temperature)) Serial.print(temperature, 2);
  Serial.print(",");
  if (!isnan(humidity)) Serial.print(humidity, 2);
  Serial.println("");

  delay(5000); // log every 5 seconds
}
