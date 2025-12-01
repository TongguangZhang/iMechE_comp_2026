#include <Servo.h>

const int SERVO_PIN = 6; 

Servo servo;

void setup() {
  Serial.begin(9600);
  Serial.println("attaching");
  servo.attach(SERVO_PIN, 1000, 2000);
  Serial.println("test pwm");
}

void loop() {
  Serial.println("min");
  servo.writeMicroseconds(1000);
  delay(3000);
  Serial.println("max");
  servo.writeMicroseconds(2000);
  delay(3000);
}
