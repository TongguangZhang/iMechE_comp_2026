#include <Encoder.h>
#include <Servo.h>

// -----
// Pins
// -----

// esc
const int ESC_PIN = 11; // 3, 5, 6, 9, 10, and 11 all should work

// encoder
const int ENCODER_PIN_A = 2;
const int ENCODER_PIN_B = 3;

// -----
// Math consts
// -----

// esc
const int MAX_THROTTLE = 2000;
const int NEUTRAL_THROTTLE = 1500;
const int MIN_THROTTLE = 1000;

// tracking wheel - in millimeters
const int MAX_DISTANCE = 3000;
const int MIN_DISTANCE = 0;
const int WHEEL_RADIUS = 100;
const int ENCODER_COUNTS_PER_REV = 1400;
// const int MAX_ENCODER_COUNT = (MAX_DISTANCE / (WHEEL_RADIUS * 2 * 3.14159)) * ENCODER_COUNTS_PER_REV;
const int MAX_ENCODER_COUNT = 1000;
const int MIN_ENCODER_COUNT = 0;

// pid constants
const float SLOWDOWN_THRESHOLD = 0.5;
const int SLOWDOWN_THRESHOLD_FORWARD = MAX_ENCODER_COUNT * SLOWDOWN_THRESHOLD;
const int SLOWDOWN_THRESHOLD_REVERSE = MIN_ENCODER_COUNT + (MAX_ENCODER_COUNT - MIN_ENCODER_COUNT) * (1 - SLOWDOWN_THRESHOLD);
const int MIN_SPEED = (MAX_THROTTLE - NEUTRAL_THROTTLE) * 0.1; // 10% of max speed
const int MIN_FORWARD_SPEED = NEUTRAL_THROTTLE + MIN_SPEED;
const int MIN_REVERSE_SPEED = NEUTRAL_THROTTLE - MIN_SPEED; 

Encoder encoder_wheel(ENCODER_PIN_A, ENCODER_PIN_B);
Servo esc;

enum class Direction {
  FORWARD,
  REVERSE,
  STOP
};

void setup() {
  Serial.begin(9600);
  Serial.println("test pwm");
  esc_arming_sequence();
  encoder_wheel.write(MIN_ENCODER_COUNT);

  // print max encoder count and min encoder count for debugging
  Serial.print("Max encoder count: ");
  Serial.println(MAX_ENCODER_COUNT);
  Serial.print("Min encoder count: ");
  Serial.println(MIN_ENCODER_COUNT);

  // print slowdown thresholds for debugging
  Serial.print("Slowdown threshold forward: ");
  Serial.println(SLOWDOWN_THRESHOLD_FORWARD);
  Serial.print("Slowdown threshold reverse: ");
  Serial.println(SLOWDOWN_THRESHOLD_REVERSE);
}

bool reached_top = false;

void loop() {
  // read encoder value
  long pos = encoder_wheel.read();
  Serial.print("Encoder count: ");
  Serial.println(pos);

  const Direction direction = reached_top ? Direction::REVERSE : Direction::FORWARD;
  if (pos >= MAX_ENCODER_COUNT) {
    reached_top = true;
  } else if (pos <= MIN_ENCODER_COUNT) {
    reached_top = false;
  }
  // do the math and write to esc
  int pwm = encoder_count_to_pwm_pid(pos, direction);
  Serial.print("mapped pwm value: ");
  Serial.println(pwm);
  esc.writeMicroseconds(pwm);
  delay(1000);
}

// arms esc
void esc_arming_sequence() {
  Serial.println("Manually setting to 0");
  pinMode(ESC_PIN, OUTPUT);
  digitalWrite(ESC_PIN, LOW);
  delay(1000);
  esc.attach(ESC_PIN, MIN_THROTTLE, MAX_THROTTLE);

  Serial.println("Sending neutral signal to arm ESC...");
  esc.writeMicroseconds(NEUTRAL_THROTTLE);

  delay(3000); 
}

int encoder_count_to_pwm_pid(int encoder_count, Direction direction) {
  if (direction == Direction::FORWARD) {
    if (encoder_count < SLOWDOWN_THRESHOLD_FORWARD) {
      return MAX_THROTTLE;
    }

    int pwm = map(encoder_count, SLOWDOWN_THRESHOLD_FORWARD, MAX_ENCODER_COUNT, MAX_THROTTLE, MIN_FORWARD_SPEED);
    return constrain(pwm, MIN_FORWARD_SPEED, MAX_THROTTLE);
  }

  if (direction == Direction::REVERSE) {
    if (encoder_count > SLOWDOWN_THRESHOLD_REVERSE) {
      return MIN_THROTTLE;
    }

    int pwm = map(encoder_count, SLOWDOWN_THRESHOLD_REVERSE, MIN_ENCODER_COUNT, MIN_THROTTLE, MIN_REVERSE_SPEED);
    return constrain(pwm, MIN_THROTTLE, MIN_REVERSE_SPEED);
  }

  return NEUTRAL_THROTTLE;
}
