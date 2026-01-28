// Arduino Pin Definitions
const int pump1Pin = 9; // Pin 9 connected to Gate of IRL44N MOSFET for Pump 1

// PWM value for medium speed
// PWM range: 0 (off) to 255 (full speed)
// 127 is about 50% duty cycle → medium speed
const int mediumSpeed = 127;

void setup() {
  // Initialize pump control pin as OUTPUT
  pinMode(pump1Pin, OUTPUT);

  // Start pump at medium speed
  analogWrite(pump1Pin, mediumSpeed); // PWM controls MOSFET → controls pump speed
}

void loop() {
  // In this simple example, the pump runs continuously at medium speed
  // If you want to vary speed or turn on/off periodically, you can use analogWrite with different values
  // Example for testing: run 5 sec on, 5 sec off
  /*
  analogWrite(pump1Pin, mediumSpeed); // Pump ON at medium speed
  delay(5000);                         // Wait 5 seconds
  analogWrite(pump1Pin, 0);            // Pump OFF
  delay(5000);                         // Wait 5 seconds
  */
}
