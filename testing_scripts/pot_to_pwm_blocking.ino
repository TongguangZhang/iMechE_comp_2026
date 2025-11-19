const int POT_IN = A0;
const int PWM_OUT = 13;

const int PWM_PERIOD = 20000;

void outputPWM(float dutyCycle) {
  digitalWrite(PWM_OUT, HIGH);
  delayMicroseconds(PWM_PERIOD * dutyCycle);
  digitalWrite(PWM_OUT, LOW);
  delay(18);
  delayMicroseconds(2000 - (PWM_PERIOD * dutyCycle));
}

void bldcStartup() {
  digitalWrite(PWM_OUT, LOW);
  
  while(true) {
    int tmp = analogRead(POT_IN);
    if(abs(tmp - 512) > 10) {
      continue;
    }

    delay(1000);
    
    if(abs(analogRead(POT_IN) - 512) <= 10) {
      break;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(PWM_OUT, OUTPUT);
  bldcStartup();
}

unsigned long lastPrint = 0;
void loop() {
  // put your main code here, to run repeatedly:
  int input = analogRead(POT_IN);
  float dutyCycle = 0.05 + 0.05 * (input / 1023.0);

  if(millis() > lastPrint + 1000) {
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "ADC Value: %d, Duty Cycle: %.3f\n", input, dutyCycle);
    Serial.write(buffer);
    lastPrint = millis();
  }
  
  outputPWM(dutyCycle);
}
