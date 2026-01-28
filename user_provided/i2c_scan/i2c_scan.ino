/***************************************************************
 * Date: 2026-01-27
 * Objective: Scan the I2C bus on the Arduino Mega 2560 and
 * report all detected I2C devices. Useful for troubleshooting
 * sensors like SHT30, BMP280, RTC modules, etc.
 ***************************************************************/

#include <Wire.h>

void setup() {
  Wire.begin();           // Initialize I2C
  Serial.begin(9600);     // Open Serial Monitor
  while (!Serial);        // Wait for Serial Monitor
  Serial.println("I2C Scanner Initialized");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning I2C bus...");

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
    // Errors 1-3: no device, ignored
  }

  if (nDevices == 0) Serial.println("No I2C devices found.");
  else Serial.println("Scan complete.");

  Serial.println("---------------------------");
  delay(5000); // Repeat scan every 5 seconds
}
