/***************************************************************
 * Date: 2026-01-27
 * Objective:
 *   - Independently control two peristaltic pumps via PWM.
 *   - Optionally read SHT30 temperature/humidity sensor.
 *   - Control a heating pad to maintain target temperature.
 *   - Print system status to Serial Monitor:
 *       Time elapsed (HH:MM:SS)
 *       Pump speeds (%)
 *       Temperature (°C)
 *       Humidity (%)
 *       Heater target (°C)
 *   - Allow user input to adjust pump speeds and heater target.
 *
 * Libraries Required:
 *   - Wire.h (built-in)
 *   - Adafruit_SHT31 (install via Library Manager)
 *
 * Hardware Setup:
 * -------------------------
 * Pump 1 (6V):
 *   - MOSFET Drain -> Pump negative
 *   - Pump positive -> 6V supply +
 *   - MOSFET Gate -> Arduino pin 9
 *   - MOSFET Source -> GND (common with Arduino)
 *   - 1N4007 diode across pump terminals (cathode to +)
 *
 * Pump 2 (6V):
 *   - MOSFET Drain -> Pump negative
 *   - Pump positive -> 6V supply +
 *   - MOSFET Gate -> Arduino pin 8
 *   - MOSFET Source -> GND (common with Arduino)
 *   - 1N4007 diode across pump terminals (cathode to +)
 *
 * Heating Pad (resistive, 5V-12V):
 *   - MOSFET Drain -> heater negative
 *   - Heater positive -> supply +
 *   - MOSFET Gate -> Arduino pin 7
 *   - MOSFET Source -> GND (common with Arduino)
 *   - Optional diode if inductive
 *
 * SHT30 Sensor:
 *   - Red -> 3.3-5V
 *   - Black -> GND
 *   - Yellow -> SCL (pin 21 on Mega)
 *   - White -> SDA (pin 20 on Mega)
 ***************************************************************/

#include <Wire.h>
#include <Adafruit_SHT31.h>

// ----------------------- Pin Definitions -----------------------
const int pump1Pin = 9;
const int pump2Pin = 8;
const int heaterPin = 7;

// ----------------------- Pump Settings ------------------------
const int maxPWM = 255;
const int minPWM = 50; // prevent stalling

int pump1SpeedPWM = 0;
int pump2SpeedPWM = 0;

// ----------------------- SHT30 -------------------------------
Adafruit_SHT31 sht30 = Adafruit_SHT31();
bool sht30Available = false;

// ----------------------- Heater Control ----------------------
float heaterTargetTemp = 40.0; // °C default
float heaterTempTolerance = 1.0;

// ----------------------- Timing ------------------------------
unsigned long startMillis;

// ----------------------- Setup -------------------------------
void setup() {
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);
  pinMode(heaterPin, OUTPUT);

  analogWrite(pump1Pin, pump1SpeedPWM);
  analogWrite(pump2Pin, pump2SpeedPWM);
  analogWrite(heaterPin, LOW);

  Serial.begin(9600);
  while (!Serial);

  startMillis = millis();

  // Initialize SHT30
  if (sht30.begin(0x44)) {
    sht30Available = true;
    Serial.println("SHT30 sensor detected.");
  } else {
    sht30Available = false;
    Serial.println("SHT30 not found. Pumps and heater will still operate.");
  }

  // Commands info
  Serial.println("Commands:");
  Serial.println("1 <0-100> -> Set Pump1 speed");
  Serial.println("2 <0-100> -> Set Pump2 speed");
  Serial.println("B <0-100> -> Set Both pumps");
  Serial.println("H <temperature> -> Set heater target °C");

  // CSV header
  Serial.println("Time(HH:MM:SS),Pump1(%),Pump2(%),Temperature(°C),Humidity(%),Heater_Target(°C)");
}

// ----------------------- Helper Functions --------------------
int mapSpeedToPWM(int speedPercent) {
  speedPercent = constrain(speedPercent, 0, 100);
  int pwmValue = map(speedPercent, 0, 100, 0, maxPWM);
  if (pwmValue > 0 && pwmValue < minPWM) pwmValue = minPWM;
  return pwmValue;
}

// ----------------------- Main Loop ---------------------------
void loop() {
  // -------- Serial Input --------
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      int spaceIndex = input.indexOf(' ');
      if (spaceIndex > 0) {
        String cmd = input.substring(0, spaceIndex);
        String valueStr = input.substring(spaceIndex + 1);
        int value = valueStr.toInt();

        if (cmd == "1") {
          pump1SpeedPWM = mapSpeedToPWM(value);
          analogWrite(pump1Pin, pump1SpeedPWM);
          Serial.print("Pump 1 on Pin 9 - Speed: "); Serial.print(value); Serial.println("%");
        }
        else if (cmd == "2") {
          pump2SpeedPWM = mapSpeedToPWM(value);
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Pump 2 on Pin 8 - Speed: "); Serial.print(value); Serial.println("%");
        }
        else if (cmd == "B" || cmd == "b") {
          pump1SpeedPWM = mapSpeedToPWM(value);
          pump2SpeedPWM = mapSpeedToPWM(value);
          analogWrite(pump1Pin, pump1SpeedPWM);
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Both pumps set to: "); Serial.print(value); Serial.println("%");
        }
        else if (cmd == "H" || cmd == "h") {
          heaterTargetTemp = (float)value;
          Serial.print("Heater target temperature set to: "); Serial.print(heaterTargetTemp); Serial.println("°C");
        }
        else {
          Serial.println("Invalid command.");
        }
      }
    }
    while (Serial.available() > 0) Serial.read(); // flush
  }

  // -------- Read SHT30 --------
  float temperature = -1.0;
  float humidity = -1.0;
  if (sht30Available) {
    temperature = sht30.readTemperature();
    humidity = sht30.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
      temperature = -1;
      humidity = -1;
    }
  }

  // -------- Heater Control --------
  if (sht30Available && temperature >= 0) {
    if (temperature < heaterTargetTemp - heaterTempTolerance) analogWrite(heaterPin, 255);
    else if (temperature > heaterTargetTemp + heaterTempTolerance) analogWrite(heaterPin, 0);
  }

  // -------- Time Elapsed --------
  unsigned long elapsedSeconds = (millis() - startMillis) / 1000;
  unsigned long hours = elapsedSeconds / 3600;
  unsigned long minutes = (elapsedSeconds % 3600) / 60;
  unsigned long seconds = elapsedSeconds % 60;

  int pump1Percent = map(pump1SpeedPWM, 0, maxPWM, 0, 100);
  int pump2Percent = map(pump2SpeedPWM, 0, maxPWM, 0, 100);

  // -------- Print Status --------
  Serial.print(hours); Serial.print(":");
  if (minutes < 10) Serial.print("0"); Serial.print(minutes); Serial.print(":");
  if (seconds < 10) Serial.print("0"); Serial.print(seconds); Serial.print(",");
  Serial.print(pump1Percent); Serial.print("%,");
  Serial.print(pump2Percent); Serial.print("%,");
  Serial.print(temperature,2); Serial.print(",");
  Serial.print(humidity,2); Serial.print(",");
  Serial.println(heaterTargetTemp,1);

  delay(5000); // log every 5s
}
