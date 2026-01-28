// ----------------------- Pin Definitions -----------------------
const int pump1Pin = 9; // Pump 1 MOSFET gate
const int pump2Pin = 8; // Pump 2 MOSFET gate

// ----------------------- PWM Settings -------------------------
const int maxPWM = 255; // Full speed PWM
const int minPWM = 50;  // Minimum PWM to prevent stalling

// ----------------------- Current Pump States -------------------
int pump1SpeedPWM = 0;  // Start OFF
int pump2SpeedPWM = 0;  // Start OFF

// ----------------------- Setup Function -----------------------
void setup() {
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);

  // Ensure both pumps are OFF initially
  analogWrite(pump1Pin, pump1SpeedPWM);
  analogWrite(pump2Pin, pump2SpeedPWM);

  // Initialize Serial for keyboard input
  Serial.begin(9600);
  Serial.println("Two-Pump Control Initialized.");
  Serial.println("Commands: PumpNumber SpeedPercent or B SpeedPercent");
  Serial.println("Pump 1 = Pin 9, Pump 2 = Pin 8");
  Serial.println("Example: 1 50  -> Pump 1 at 50%");
  Serial.println("         2 75  -> Pump 2 at 75%");
  Serial.println("         B 30  -> Both pumps at 30%");
}

// ----------------------- Main Loop ----------------------------
void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read full line
    input.trim(); // Remove whitespace

    if (input.length() > 0) {
      int spaceIndex = input.indexOf(' '); // Separate pump identifier and speed
      if (spaceIndex > 0) {
        String pumpStr = input.substring(0, spaceIndex);
        String speedStr = input.substring(spaceIndex + 1);
        speedStr.trim();

        int speedPercent = speedStr.toInt(); // Convert speed to integer

        // Validate speed
        if (speedPercent < 0) speedPercent = 0;
        if (speedPercent > 100) speedPercent = 100;

        // Map 0-100% speed to PWM 0-255
        int pwmValue = map(speedPercent, 0, 100, 0, maxPWM);
        if (pwmValue > 0 && pwmValue < minPWM) pwmValue = minPWM;

        // ----------------------- Pump 1 -----------------------
        if (pumpStr == "1") {
          pump1SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          Serial.print("Pump 1 on Pin 9 - Speed set to: ");
          Serial.print(speedPercent);
          Serial.print("% (PWM = ");
          Serial.print(pump1SpeedPWM);
          Serial.println(")");
        }
        // ----------------------- Pump 2 -----------------------
        else if (pumpStr == "2") {
          pump2SpeedPWM = pwmValue;
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Pump 2 on Pin 8 - Speed set to: ");
          Serial.print(speedPercent);
          Serial.print("% (PWM = ");
          Serial.print(pump2SpeedPWM);
          Serial.println(")");
        }
        // ----------------------- Both Pumps -------------------
        else if (pumpStr == "B" || pumpStr == "b") {
          pump1SpeedPWM = pwmValue;
          pump2SpeedPWM = pwmValue;
          analogWrite(pump1Pin, pump1SpeedPWM);
          analogWrite(pump2Pin, pump2SpeedPWM);
          Serial.print("Both pumps set to: ");
          Serial.print(speedPercent);
          Serial.print("% (PWM = ");
          Serial.print(pwmValue);
          Serial.println(")");
        }
        else {
          Serial.println("Invalid pump identifier. Use 1, 2, or B for both.");
        }
      }
      else {
        Serial.println("Invalid input format. Example: 1 50 or B 30");
      }
    }

    // Clear any remaining characters
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  delay(50); // Small delay to stabilize loop
}
