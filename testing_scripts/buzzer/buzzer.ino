// Pins
const int BUZZER_PIN = 13;

void setup() {
  Serial.begin(9600);
  Serial.println("Buzzer test");

  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // read encoder value
  Serial.println("Buzzer ON");
  tone(BUZZER_PIN, 1000);
  delay(1000);
  Serial.println("Buzzer OFF");
  noTone(BUZZER_PIN);
  delay(1000);
}
