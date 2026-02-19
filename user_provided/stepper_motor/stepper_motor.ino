/* 
  Date: 2026-02-17
  Objective: Control a NEMA 17 stepper motor with variable speed and direction via Serial Monitor
  Wiring:
    - Arduino Pin 7 -> DM542 PUL-
    - Arduino Pin 6 -> DM542 DIR-
    - PUL+ and DIR+ -> Arduino 5V
    - ENA disconnected or ENA- tied LOW (GND)
    - DM542 A+/A-, B+/B- -> stepper motor coils
    - DM542 V+/V- -> 24V DC power supply
    - DIP switches: SW1=ON, SW2=OFF, SW3=ON (2A); SW4=ON, SW5=ON, SW6=OFF, SW7=OFF (1/8 microstep); SW8=OFF
*/

const int stepPin = 7;  // PUL-
const int dirPin  = 6;  // DIR-

// Stepper control variables
int motorSpeedPercent = 0;     // Speed 0-100%
bool directionForward = true;   // Direction
float currentDelay = 2000;      // Current microseconds per step
float targetDelay = 2000;       // Target delay for smooth acceleration
const float minDelay = 500;     // Fastest speed
const float maxDelay = 4000;    // Slowest speed
const float accelStep = 5;      // Microseconds per loop for acceleration

void setup() {
  Serial.begin(9600);            // Start Serial Monitor
  pinMode(stepPin, OUTPUT);      // Step pin
  pinMode(dirPin, OUTPUT);       // Direction pin
  digitalWrite(dirPin, HIGH);    // Forward

  Serial.println("Stepper Motor Controller Initialized.");
  Serial.println("Commands:");
  Serial.println("  MXX -> Set speed (0-100%)");
  Serial.println("  F   -> Forward direction");
  Serial.println("  R   -> Reverse direction");
  Serial.println("Motor starts OFF.");
}

void loop() {
  // --- Serial commands ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();  // Make commands case-insensitive

    if (input.startsWith("M")) {
      int speed = input.substring(1).toInt();
      if (speed < 0) speed = 0;
      if (speed > 100) speed = 100;
      motorSpeedPercent = speed;

      // Map speed % to step delay (0% = slowest, 100% = fastest)
      targetDelay = map(motorSpeedPercent, 0, 100, maxDelay, minDelay);

      Serial.print("Motor speed set to ");
      Serial.print(motorSpeedPercent);
      Serial.println("%");
    } else if (input == "F") {
      directionForward = true;
      digitalWrite(dirPin, HIGH);
      Serial.println("Direction: Forward");
    } else if (input == "R") {
      directionForward = false;
      digitalWrite(dirPin, LOW);
      Serial.println("Direction: Reverse");
    } else {
      Serial.println("Unknown command. Use MXX, F, or R.");
    }
  }

  // --- Smooth acceleration/deceleration ---
  if (currentDelay < targetDelay) {
    currentDelay += accelStep;        // Slow down smoothly
    if (currentDelay > targetDelay) currentDelay = targetDelay;
  } else if (currentDelay > targetDelay) {
    currentDelay -= accelStep;        // Speed up smoothly
    if (currentDelay < targetDelay) currentDelay = targetDelay;
  }

  // --- Step motor if speed > 0 ---
  if (motorSpeedPercent > 0) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(currentDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(currentDelay);
  } else {
    // Motor OFF, short delay to avoid CPU overload
    delay(100);
  }
}
