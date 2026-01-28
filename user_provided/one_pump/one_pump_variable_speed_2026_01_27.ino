// Arduino Pump Control with Keyboard Input
// Pin 9 controls Pump 1 via IRL44N MOSFET
// Speed adjustable from 0 (off) to 10 (full speed) via Serial Monitor

const int pump1Pin = 9;       // Pin connected to MOSFET gate
const int maxPWM = 255;       // Maximum PWM value for full speed
const int minPWM = 50;        // Minimum PWM for low speed (avoid stalling)

void setup() {
  pinMode(pump1Pin, OUTPUT);   // Set MOSFET gate as output
  analogWrite(pump1Pin, minPWM); // Start pump at low speed

  Serial.begin(9600);          // Initialize Serial communication
  Serial.println("Pump Control Ready.");
  Serial.println("Enter speed from 0 (stopped) to 10 (max).");
}

void loop() {
  // Check if data is available from Serial Monitor
  if (Serial.available() > 0) {
    char inputChar = Serial.read(); // Read one character from keyboard

    // Only accept characters '0' to '10'
    if (inputChar >= '0' && inputChar <= '9') {
      int speedLevel = inputChar - '0'; // Convert ASCII char to integer 0-9

      // Map speed level (0-10) to PWM range
      // 0 -> 0 PWM, 1 -> minPWM, 10 -> maxPWM
      int pwmValue = map(speedLevel, 0, 10, 0, maxPWM);

      // Ensure minimum PWM is not below minPWM for speed 1-10
      if (pwmValue > 0 && pwmValue < minPWM) {
        pwmValue = minPWM;
      }

      analogWrite(pump1Pin, pwmValue); // Set pump speed

      // Feedback to user
      Serial.print("Speed set to: ");
      Serial.print(speedLevel);
      Serial.print(" (PWM = ");
      Serial.print(pwmValue);
      Serial.println(")");
    } else {
      Serial.println("Invalid input. Enter 0-10.");
    }

    // Clear any remaining characters in buffer
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  // Optional: add a small delay to stabilize loop
  delay(50);
}
