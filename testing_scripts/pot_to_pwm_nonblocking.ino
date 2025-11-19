#include <Servo.h>

// Pins
const int POT_PIN = A0;
const int ESC_PIN = 13; // 3, 5, 6, 9, 10, 11, 13 all should work, but 9 and 10 are sus

// Math consts
const int MAX_POT_VALUE = 1023;
const int MIN_POT_VALUE = 0;

const int MAX_THROTTLE = 2000;
const int NEUTRAL_THROTTLE = 1500;
const int MIN_THROTTLE = 1000;

Servo esc;

void setup() {
  Serial.begin(9600);
  Serial.println("test pwm");

  // arming sequence
  esc.attach(ESC_PIN, MIN_THROTTLE, MAX_THROTTLE);

  Serial.println("Sending neutral signal to arm ESC...");
  esc.writeMicroseconds(NEUTRAL_THROTTLE);

  delay(3000);
}

void loop() {
  // read pot value
  int pot_val = analogRead(POT_PIN);
  Serial.print("pot value: ");
  Serial.println(pot_val);

  // do the math and write to esc
  int pwm = constrain(map(pot_val, MIN_POT_VALUE, MAX_POT_VALUE, MIN_THROTTLE, MAX_THROTTLE), MIN_THROTTLE, MAX_THROTTLE);
  Serial.print("mapped pwm value: ");
  Serial.println(pwm);
  esc.writeMicroseconds(pwm);
}
