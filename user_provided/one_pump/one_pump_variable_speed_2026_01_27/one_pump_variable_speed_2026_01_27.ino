// Arduino Pump Control Sketch
// Pump 1 connected to Pin 9 via IRL44N MOSFET
// DC6V Peristaltic Pump
// Keyboard input over Serial: "PumpNumber SpeedPercent" (e.g., "1 50")

// ----------------------- Pin Definitions -----------------------
const int pump1Pin = 9;        // Arduino pin connected to MOSFET gate for Pump 1

// ----------------------- PWM Settings -------------------------
const int maxPWM = 255;        // Full speed PWM value
const int minPWM = 50;         // Minimum PWM to prevent pump stalling at very low speeds

// ----------------------- Current Pump State -------------------
int pump1SpeedPWM = 0;         // Start pump OFF (0 PWM)

// ----------------------- Setup Function -----------------------
void setup() {
  pinMode(pump1Pin, OUTPUT);       // Set MOSFET gate as output
  analogWrite(pump1Pin, pump1SpeedPWM); // Ensure Pump 1 is OFF initially

  // Initialize Serial communication
  Serial.begin(9600);
  Serial.println("Pump Control Initialized.");
  Serial.println("Pump 1 on Pin 9");
  Serial.println("Format: PumpNumber SpeedPercent (0=off, 100=max)");
  Serial.println("Example: 1 50 sets Pump 1 to 50% speed.");
}

// ----------------------- Main Loop ----------------------------
void loop() {
  // Check if data is available on Serial
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read line until Enter
    input.trim(); // Remove whitespace

    if (input.length() > 0) {
      int spaceIndex = input.indexOf(' '); // Separate PumpNumber and SpeedPercent
      if (spaceIndex > 0) {
        String pumpStr = input.substring(0, spaceIndex);
        String speedStr = input.substring(spaceIndex + 1);

        int pumpNum = pumpStr.toInt();        // Pump number
        int speedPercent = speedStr.toInt();  // Speed percentage 0-100

        // Validate input
        if (pumpNum == 1 && speedPercent >= 0 && speedPercent <= 100) {
          // Map speedPercent (0-100) to PWM value (0-255)
          int pwmValue = map(speedPercent, 0, 100, 0, maxPWM);

          // Ensure minimum PWM is at least minPWM for speeds 1-100
          if (pwmValue > 0 && pwmValue < minPWM) {
            pwmValue = minPWM;
          }

          // Set Pump 1 speed
          analogWrite(pump1Pin, pwmValue);
          pump1SpeedPWM = pwmValue;

          // Print feedback to Serial Monitor
          Serial.print("Pump 1 on Pin 9 - Speed set to: ");
          Serial.print(speedPercent);
          Serial.print("% (PWM = ");
          Serial.print(pwmValue);
          Serial.println(")");

        } else {
          Serial.println("Invalid pump number or speed. PumpNumber=1, Speed=0-100");
        }
      } else {
        Serial.println("Invalid format. Use: PumpNumber SpeedPercent (e.g., 1 50)");
      }
    }

    // Clear any remaining characters in Serial buffer
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  delay(50); // Small delay to stabilize loop
}
