// Pins
const int ENCODER_PIN_A = 2;
const int ENCODER_PIN_B = 3;

volatile int count_interrupt_A = 0;
volatile int count_interrupt_B = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Interrupt test");
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), interruptA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), interruptB, RISING);
}

void loop() {
  Serial.print("Encoder count A: ");
  Serial.print(count_interrupt_A);
  Serial.print(" Encoder count B: ");
  Serial.println(count_interrupt_B);
  delay(3000);
}

void interruptA() {
  count_interrupt_A++;
  Serial.println("Interrupt A fired");
}

void interruptB() {
  count_interrupt_B++;
  Serial.println("Interrupt B fired");
}
