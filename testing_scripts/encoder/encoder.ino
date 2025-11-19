#include <Encoder.h>

// Pins
const int ENCODER_PIN_A = 22;
const int ENCODER_PIN_B = 24;

Encoder encoderWheel(ENCODER_PIN_A, ENCODER_PIN_B);

void setup() {
  Serial.begin(9600);
  Serial.println("Encoder test");

  encoderWheel.write(0);
}

void loop() {
  // read encoder value
  long pos = encoderWheel.read();
  Serial.print("Encoder count: ");
  Serial.println(pos);
  delay(1000);
}
