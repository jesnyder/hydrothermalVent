/***************************************************************
 * Date: 2026-01-27
 * Objective: Control two peristaltic pumps independently,
 * read temperature and humidity from SHT30 sensor,
 * and print CSV-style logs to the Serial Monitor.
 *
 * Features:
 * - Pump 1 on Pin 9
 * - Pump 2 on Pin 8
 * - Pumps start OFF
 * - Keyboard input 0-100% speed
 * - Independent or simultaneous pump control
 * - Real-time CSV-style logging with elapsed time
 * - Terminal output shows system status
 *
 * Hardware Setup:
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
 *   - SDA -> Arduino SDA (Pin 20 on Mega)
 *   - SCL -> Arduino SCL (Pin 21 on Mega)
 *   - VCC -> 3.3V or 5V
 *   - GND -> GND
 *
 * Logging:
 *   - CSV-style output to Serial Monitor:
 *     Time_s,Temp_C,Humidity_%,Pump1_Speed%,Pump2_Speed%
 *   - You can save Serial Monitor output as a CSV file.
 ***************************************************************/

#include <Wire.h>
#include <Adafruit_SHT31.h>

// ----------------------- Pin Definitions -----------------------
const int pump1Pin = 9;  // Pump 1 MOSFET gate
const int pump2Pin = 8;  // Pump 2 MOSFET gate

// ----------------------- PWM Settings -------------------------
const int maxPWM = 255; // Max PWM for full speed
const int minPWM = 50;  // Minimum PWM to avoid pump stalling

// ----------------------- Pump States --------------------------
int pump1SpeedPWM = 0;  // Pump 1 OFF initially
int pump2SpeedPWM = 0;  // Pump 2 OFF initially

// ----------------------- SHT30 Sensor -------------------------
Adafruit_SHT31 sht30 = Adafruit_SHT31();

// ----------------------- Time Tracking ------------------------
unsigned long startTime = 0;

// ----------------------- Setup Function -----------------------
void setup() {
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);

  analogWrite(pump1Pin, pump1SpeedPWM);
  analogWrite(pump2Pin, pump2SpeedPWM);

  Serial.begin(9600);
  Serial.println("Two-Pump Control with SHT30 Logging Initialized.");
  startTime = millis();

  // Initialize SHT30
  if (!sht30.begin(0x44)) {
    Serial.println("SHT30 not found. Check wiring!");
    while (1); // Halt if sensor not found
  }

  // Print CSV header to Serial Monitor
  Serial.println("Time_s,Temp_C,Humidity_%,Pump1_Speed%,Pump2_Speed%");
  Serial.println("Commands:");
  Serial.println("  1 <0-100>   -> Pump 1 speed");
  Serial.println("  2 <0-100>   -> Pump 2 speed");
  Serial.println("  B <0-100>   -> Both pumps same speed");
}

// ----------------------- Main Loop ----------------------------
void loop() {
  // ----------------- Handle Serial Input -----------------
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

        // Map 0-100% speed to 0-255 PWM
        int pwmValue = map(speedPercent, 0, 100, 0, maxPWM);
        if (pwmValue > 0 && pwmValue < minPWM) pwmValue = minPWM;

        if (pumpStr == "1") {
          pump1SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          Serial.print("Pump 1 on Pin 9 - Speed: ");
          Serial.print(speedPercent);
          Serial.println("%");
        } else if (pumpStr == "2") {
          pump2SpeedPWM = pwmValue;
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Pump 2 on Pin 8 - Speed: ");
          Serial.print(speedPercent);
          Serial.println("%");
        } else if (pumpStr == "B" || pumpStr == "b") {
          pump1SpeedPWM = pwmValue;
          pump2SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Both pumps set to: ");
          Serial.print(speedPercent);
          Serial.println("%");
        } else {
          Serial.println("Invalid pump identifier. Use 1, 2, or B.");
        }
      } else {
        Serial.println("Invalid input format. Example: '1 50'");
      }
    }
    // Clear any extra input
    while (Serial.available() > 0) Serial.read();
  }

  // ----------------- Read SHT30 --------------------------
  float temperature = sht30.readTemperature();
  float humidity = sht30.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    // Convert PWM back to percent for logging
    int pump1Percent = map(pump1SpeedPWM, 0, maxPWM, 0, 100);
    int pump2Percent = map(pump2SpeedPWM, 0, maxPWM, 0, 100);

    // Calculate elapsed time
    unsigned long elapsedTime = (millis() - startTime) / 1000;

    // CSV-style print: Time, Temp, Humidity, Pump1%, Pump2%
    Serial.print(elapsedTime);
    Serial.print(",");
    Serial.print(temperature, 2);
    Serial.print(",");
    Serial.print(humidity, 2);
    Serial.print(",");
    Serial.print(pump1Percent);
    Serial.print(",");
    Serial.println(pump2Percent);

    // Also print system status
    Serial.print("Status: Time=");
    Serial.print(elapsedTime);
    Serial.print("s, Temp=");
    Serial.print(temperature, 2);
    Serial.print("C, Humidity=");
    Serial.print(humidity, 2);
    Serial.print("%, Pump1=");
    Serial.print(pump1Percent);
    Serial.print("%, Pump2=");
    Serial.print(pump2Percent);
    Serial.println("%");
  } else {
    Serial.println("Failed to read from SHT30");
  }

  delay(5000); // Log every 5 seconds
}
