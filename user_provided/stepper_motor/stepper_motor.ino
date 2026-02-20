/* 
  Date: 2026-02-17
  Objective: Control a NEMA 17 stepper motor with variable speed, direction, and endstop safety via Serial Monitor
  Wiring:
    - Arduino Pin 7 -> DM542 PUL-
    - Arduino Pin 6 -> DM542 DIR-
    - PUL+ and DIR+ -> Arduino 5V
    - ENA disconnected or ENA- tied LOW (GND)
    - DM542 A+/A-, B+/B- -> stepper motor coils
      Stepper motor wire colors:
        - A+ : Black
        - A- : Green
        - B+ : Red
        - B- : Blue
    - DM542 V+/V- -> 24V DC power supply
    - Limit Switch (Endstop):
        RED wire - COM -> Arduino GND
        BLACK wire - NC  -> Arduino Pin 9
    - DIP switches: SW1=ON, SW2=OFF, SW3=ON (2A); SW4=ON, SW5=ON, SW6=OFF, SW7=OFF (1/8 microstep); SW8=OFF
*/

const int stepPin = 7;       // PUL- (step pulse output)
const int dirPin  = 6;       // DIR- (direction control)
const int endstopPin = 9;    // Endstop input (black wire, NC)

int motorSpeedPercent = 0;    // Motor speed 0-100%
bool directionForward = true; // Motor direction
float currentDelay = 2000;    // Current step delay in microseconds
float targetDelay = 2000;     // Target delay for smooth acceleration
const float minDelay = 500;   // Minimum delay (fastest speed)
const float maxDelay = 4000;  // Maximum delay (slowest speed)
const float accelStep = 5;    // Microseconds per loop for smooth ramping

void setup() {
  Serial.begin(9600);                  // Start Serial Monitor
  pinMode(stepPin, OUTPUT);            // Step pin output
  pinMode(dirPin, OUTPUT);             // Direction pin output
  pinMode(endstopPin, INPUT_PULLUP);   // Endstop input with internal pullup (NC switch)

  digitalWrite(dirPin, HIGH);          // Start with forward direction

  // Serial instructions
  Serial.println("Stepper Motor Controller Initialized.");
  Serial.println("Commands:");
  Serial.println("  MXX -> Set speed (0-100%)");
  Serial.println("  F   -> Forward direction");
  Serial.println("  R   -> Reverse direction");
  Serial.println("Motor starts OFF.");
}

void loop() {
  // --- Serial command handling ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();  // Case-insensitive commands

    if (input.startsWith("M")) {
      int speed = input.substring(1).toInt();
      if (speed < 0) speed = 0;
      if (speed > 100) speed = 100;
      motorSpeedPercent = speed;

      // Map speed percent to microseconds per step
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

  // --- Endstop check ---
  if (digitalRead(endstopPin) == LOW) { // NC switch pressed
    motorSpeedPercent = 0;              // Stop motor immediately
    Serial.println("Endstop triggered! Motor stopped and reversing.");

    // Reverse direction
    directionForward = !directionForward;
    digitalWrite(dirPin, directionForward ? HIGH : LOW);

    delay(200); // debounce to prevent double-trigger
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
    delay(100); // Motor off, short delay to reduce CPU usage
  }
}
