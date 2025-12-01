#include <Adafruit_NeoPixel.h>
#include <Encoder.h>
#include <Servo.h>

// -----
// Config
// -----

bool START_BUTTON_RESETS = false;

// -----
// Pins
// -----

// esc
const int ESC_PIN = 11; // 3, 5, 6, 9, 10, and 11 all should work

// encoder
const int ENCODER_PIN_A = 2;
const int ENCODER_PIN_B = 3;

// buttons
const int START_BUTTON_PIN = 4;
const int TOP_LIMIT_SWITCH_PIN = 5;

// buzzer
const int BUZZER_PIN = 13;

// led
const int LED_PIN = 10;

// chain servo
const int SERVO_PIN = 6;


// -----
// Interfaces
// -----
Servo esc;

Encoder encoder_wheel(ENCODER_PIN_A, ENCODER_PIN_B);

const int NUM_RED_LEDS = 2;
const int NUM_GREEN_LEDS = 3;
Adafruit_NeoPixel pixels(NUM_RED_LEDS + NUM_GREEN_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

Servo chain_holder;

// -----
// Consts
// -----

// time (in milliseconds)
const int TIME_STOPPED_AT_TOP = 15000;
const int TIME_STOPPED_AT_BOTTOM = 5000;
const int TIME_STOPPED_AT_INTERMEDIATE = 5000;

// esc
const int MAX_THROTTLE = 2000;
const int NEUTRAL_THROTTLE = 1500;
const int MIN_THROTTLE = 1000;

// tracking wheel distances in millimeters
const int TOP_DISTANCE = 1800;
const int INTERMEDIATE_DISTANCE = 900;
const int WHEEL_RADIUS = 100;

// tracking wheel encoder counts
const int MIN_ENCODER_COUNT = 0;
const int ENCODER_COUNTS_PER_REV = 1400;
// const int TOP_ENCODER_COUNT = (TOP_DISTANCE / (WHEEL_RADIUS * 2 * 3.14159)) * ENCODER_COUNTS_PER_REV;
// const int INTERMEDIATE_ENCODER_COUNT = (INTERMEDIATE_DISTANCE / (WHEEL_RADIUS * 2 * 3.14159)) * ENCODER_COUNTS_PER_REV;;
const int TOP_ENCODER_COUNT = 1000;
const int INTERMEDIATE_ENCODER_COUNT = 500;

// "pid" constants
const float SLOWDOWN_THRESHOLD = 0.5;
const int MIN_SPEED = (MAX_THROTTLE - NEUTRAL_THROTTLE) * 0.1; // 10% of max speed
const int MIN_FORWARD_SPEED = NEUTRAL_THROTTLE + MIN_SPEED;
const int MIN_REVERSE_SPEED = NEUTRAL_THROTTLE - MIN_SPEED;
const int MAX_FORWARD_SPEED = NEUTRAL_THROTTLE + 150;
const int MAX_REVERSE_SPEED = NEUTRAL_THROTTLE - 150;
const int RAMP_UP_TIME = 3000;

// led
const uint32_t RED = pixels.Color(255, 0, 0);
const uint32_t GREEN = pixels.Color(0, 255, 0);
const uint32_t OFF = pixels.Color(0, 0, 0);

// chain servo
const int SERVO_OPEN_PWM = 1000;
const int SERVO_CLOSED_PWM = 2000;

enum class State {
  START,
  CLIMB_TO_TOP,
  STOP_AT_TOP,
  RETURN_TO_BOTTOM,
  STOP_AT_BOTTOM,
  CLIMB_TO_INTERMEDIATE,
  STOP_AT_INTERMEDIATE,
  END
};

enum class Direction {
  FORWARD,
  REVERSE,
  STOP
};

// -----
// Global variables
// ----
State current_state = State::START;
unsigned long time_of_last_state_change = 0;

// Setup code
void setup() {
  Serial.begin(9600);
  Serial.println("Main system start");

  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOP_LIMIT_SWITCH_PIN, INPUT_PULLUP);
  Serial.println("Button pins initialized");

  esc_arming_sequence();
  Serial.println(">>> ESC arming complete <<<");

  encoder_wheel.write(MIN_ENCODER_COUNT);
  Serial.println("Encoder reset to 0");

  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Buzzer pin initialized");

  chain_holder.attach(SERVO_PIN, SERVO_OPEN_PWM, SERVO_CLOSED_PWM);
  chain_holder.write(SERVO_CLOSED_PWM);
  Serial.println("Chain servo attachced");

  pixels.begin();
  pixels.setBrightness(10);
  pixels.show();
  Serial.println("Neopixel initialization");
}

// Loop code
void loop() {
  // Read the sensor and timing values
  Serial.println("--- Reading sensors ---");

  // Time
  const long current_time = millis();
  const long time_from_last_state_change = current_time - time_of_last_state_change;
  Serial.print("time from last state change: ");
  Serial.print(time_from_last_state_change);
  Serial.print(" | current time: ");
  Serial.print(current_time);
  Serial.print(" | time of last state change: ");
  Serial.println(time_of_last_state_change);

  // Buttons
  const bool start_button_pressed = digitalRead(START_BUTTON_PIN) == LOW;
  const bool top_limit_switch_triggered = digitalRead(TOP_LIMIT_SWITCH_PIN) == LOW;
  Serial.print("Start button pressed: ");
  Serial.println(start_button_pressed ? "YES" : "NO");
  Serial.print("Top limit switch triggered: ");
  Serial.println(top_limit_switch_triggered ? "YES" : "NO");

  // Encoder
  const long encoder_count = encoder_wheel.read();
  Serial.print("Encoder count: ");
  Serial.println(encoder_count);

  Serial.println("--- Sensors read complete ---");

  // Determine the state and perform actions
  State new_state = determine_state(start_button_pressed, top_limit_switch_triggered, encoder_count, time_from_last_state_change);
  if (new_state != current_state) {
    Serial.print(">>> State changed <<<");
    current_state = new_state;
    time_of_last_state_change = current_time;
  }
  Serial.println("--- State determined ---");
  action_at_state(encoder_count);
  Serial.println("--- State actions performed ---");
}

// -----
// Drivers
// -----

// arms esc
void esc_arming_sequence() {
  Serial.println(">>> ESC arming begin <<<");
  Serial.println("Manually setting ESC pin to 0");
  pinMode(ESC_PIN, OUTPUT);
  digitalWrite(ESC_PIN, LOW);
  delay(1000);
  esc.attach(ESC_PIN, MIN_THROTTLE, MAX_THROTTLE);

  Serial.println("Sending neutral signal to arm ESC");
  esc.writeMicroseconds(NEUTRAL_THROTTLE);

  delay(3000);
}

// Maps encoder count to how fast to run the BLDC
// Slows down as it approaches the target position and is past the threshold
void set_esc_speed(long encoder_count, long target, Direction direction, int time_from_last_state_change) {
  if (direction == Direction::FORWARD) {
    const long slowdown_threshold_ticks = SLOWDOWN_THRESHOLD * (target - MIN_ENCODER_COUNT) + MIN_ENCODER_COUNT;
    if (encoder_count < slowdown_threshold_ticks) {
      // Ramp to max speed
      if (time_from_last_state_change < RAMP_UP_TIME) {
        int pwm = map(time_from_last_state_change, 0, RAMP_UP_TIME, NEUTRAL_THROTTLE, MAX_FORWARD_SPEED);
        int safe_pwm = constrain(pwm, NEUTRAL_THROTTLE, MAX_FORWARD_SPEED);
        esc.writeMicroseconds(safe_pwm);
        return;
      }
      esc.writeMicroseconds(MAX_FORWARD_SPEED);
      return;
    }

    int pwm = map(encoder_count, slowdown_threshold_ticks, target, MAX_FORWARD_SPEED, MIN_FORWARD_SPEED);
    int safe_pwm = constrain(pwm, MIN_FORWARD_SPEED, MAX_FORWARD_SPEED);
    esc.writeMicroseconds(safe_pwm);
    return;
  }

  if (direction == Direction::REVERSE) {
    const long slowdown_threshold_ticks_reverse = (1 - SLOWDOWN_THRESHOLD) * (target - MIN_ENCODER_COUNT) + MIN_ENCODER_COUNT;
    if (encoder_count > slowdown_threshold_ticks_reverse) {
      // Ramp to max speed
      if (time_from_last_state_change < RAMP_UP_TIME) {
        int pwm = map(time_from_last_state_change, 0, RAMP_UP_TIME, NEUTRAL_THROTTLE, MAX_REVERSE_SPEED);
        int safe_pwm = constrain(pwm, MAX_REVERSE_SPEED, NEUTRAL_THROTTLE);
        esc.writeMicroseconds(safe_pwm);
        return;
      }
      esc.writeMicroseconds(MAX_REVERSE_SPEED);
      return;
    }

    int pwm = map(encoder_count, slowdown_threshold_ticks_reverse, MIN_ENCODER_COUNT, MAX_REVERSE_SPEED, MIN_REVERSE_SPEED);
    int safe_pwm = constrain(pwm, MAX_REVERSE_SPEED, MIN_REVERSE_SPEED);
    esc.writeMicroseconds(safe_pwm);
    return;
  }

  esc.writeMicroseconds(NEUTRAL_THROTTLE);
}

// Turn on or off
void buzzer_state(bool buzzer_on) {
  if (buzzer_on) {
    tone(BUZZER_PIN, 1000);
    return;
  }
  noTone(BUZZER_PIN);
}

// Turn green and red lights on or off
void neopixel_state(bool red_on, bool green_on) {
  for (int i = 0; i < NUM_RED_LEDS; i++) {
    if (red_on) {
      pixels.setPixelColor(i, RED);
    } else {
      pixels.setPixelColor(i, OFF);
    }
  }
  for (int i = NUM_RED_LEDS; i < NUM_RED_LEDS + NUM_GREEN_LEDS; i++) {
    if (green_on) {
      pixels.setPixelColor(i, GREEN);
    } else {
      pixels.setPixelColor(i, OFF);
    }
  }
  pixels.show();
}

// Servo open or closed
void chain_servo_state(bool open) {
  if (open) {
    chain_holder.write(SERVO_OPEN_PWM); // open position
    return;
  }
  chain_holder.write(SERVO_CLOSED_PWM); // closed position
}

// -----
// State machine logic
// -----

// Based on the inputs and current state, determine the next state
State determine_state(bool start_button_pressed, bool top_limit_switch_triggered, long encoder_count, int time_from_last_state_change) {
  Serial.println("--- Determining state ---");
  if (current_state != State::START && START_BUTTON_RESETS && start_button_pressed) {
    Serial.println(">>> Current State: not START, but start button pressed and reset config is on");
    Serial.println(">>> Resetting to START state <<<");
    return State::START;
  }
  switch (current_state) {
    case State::START: {
      Serial.println(">>> Current State: START <<<");
      bool go_to_next_stage = start_button_pressed;
      if (go_to_next_stage) {
        return State::CLIMB_TO_TOP;
      }
      Serial.println(">>> Staying in state: START <<<");
      break;
    }
    case State::CLIMB_TO_TOP: {
      Serial.println(">>> Current State: CLIMB_TO_TOP <<<");
      bool go_to_next_stage = top_limit_switch_triggered;
      if (go_to_next_stage) {
        return State::STOP_AT_TOP;
      }
      Serial.println(">>> Staying in state: CLIMB_TO_TOP <<<");
      break;
    }
    case State::STOP_AT_TOP: {
      Serial.println(">>> Current State: STOP_AT_TOP <<<");
      bool go_to_next_stage = time_from_last_state_change > TIME_STOPPED_AT_TOP;
      if (go_to_next_stage) {
        return State::RETURN_TO_BOTTOM;
      }
      Serial.println(">>> Staying in state: STOP_AT_TOP <<<");
      break;
    }
    case State::RETURN_TO_BOTTOM: {
      Serial.println(">>> Current State: RETURN_TO_BOTTOM <<<");
      bool go_to_next_stage = encoder_count <= MIN_ENCODER_COUNT;
      if (go_to_next_stage) {
        return State::STOP_AT_BOTTOM;
      }
      Serial.println(">>> Staying in state: RETURN_TO_BOTTOM <<<");
      break;
    }
    case State::STOP_AT_BOTTOM: {
      Serial.println(">>> Current State: STOP_AT_BOTTOM <<<");
      bool go_to_next_stage = time_from_last_state_change > TIME_STOPPED_AT_BOTTOM;
      if (go_to_next_stage) {
        return State::CLIMB_TO_INTERMEDIATE;
      }
      Serial.println(">>> Staying in state: STOP_AT_BOTTOM <<<");
      break;
    }
    case State::CLIMB_TO_INTERMEDIATE: {
      bool go_to_next_stage = encoder_count >= INTERMEDIATE_ENCODER_COUNT;
      if (go_to_next_stage) {
        return State::STOP_AT_INTERMEDIATE;
      }
      Serial.println(">>> Staying in state: CLIMB_TO_INTERMEDIATE <<<");
      break;
    }
    case State::STOP_AT_INTERMEDIATE: {
      Serial.println(">>> Current State: STOP_AT_INTERMEDIATE <<<");
      bool go_to_next_stage = time_from_last_state_change > TIME_STOPPED_AT_INTERMEDIATE;
      if (go_to_next_stage) {
        return State::END;
      }
      Serial.println(">>> Staying in state: STOP_AT_INTERMEDIATE <<<");
      break;
    }
    case State::END: {
      Serial.println(">>> Current State: END <<<");
      Serial.println(">>> Staying in state: END <<<");
      break;
    }
    default: {
      Serial.println("Error: Unknown state");
      break;
    }
  }
  return current_state;
}

// Based on the current state, perform the appropriate actions for the actuators
void action_at_state(long encoder_count) {
  Serial.println("--- State actions ---");
  switch (current_state) {
    case State::START: {
      Serial.println(">>> Current State: START <<<");
      set_esc_speed(encoder_count, MIN_ENCODER_COUNT, Direction::STOP);
      buzzer_state(false);
      neopixel_state(false, false);
      chain_servo_state(false);
      break;
    }
    case State::CLIMB_TO_TOP: {
      Serial.println(">>> Current State: CLIMB_TO_TOP <<<");
      set_esc_speed(encoder_count, TOP_ENCODER_COUNT, Direction::FORWARD);
      buzzer_state(false);
      neopixel_state(false, true); // green on
      chain_servo_state(false);
      break;
    }
    case State::STOP_AT_TOP: {
      Serial.println(">>> Current State: STOP_AT_TOP <<<");
      set_esc_speed(encoder_count, TOP_ENCODER_COUNT, Direction::STOP);
      buzzer_state(true);
      neopixel_state(true, false); // red on
      chain_servo_state(false);
      break;
    }
    case State::RETURN_TO_BOTTOM: {
      Serial.println(">>> Current State: RETURN_TO_BOTTOM <<<");
      set_esc_speed(encoder_count, MIN_ENCODER_COUNT, Direction::REVERSE);
      buzzer_state(false);
      neopixel_state(false, true); // green on
      chain_servo_state(false);
      break;
    }
    case State::STOP_AT_BOTTOM: {
      Serial.println(">>> Current State: STOP_AT_BOTTOM <<<");
      set_esc_speed(encoder_count, MIN_ENCODER_COUNT, Direction::STOP);
      buzzer_state(true);
      neopixel_state(true, false); // red on
      chain_servo_state(false);
      break;
    }
    case State::CLIMB_TO_INTERMEDIATE: {
      Serial.println(">>> Current State: CLIMB_TO_INTERMEDIATE <<<");
      set_esc_speed(encoder_count, INTERMEDIATE_ENCODER_COUNT, Direction::FORWARD);
      buzzer_state(false);
      neopixel_state(false, true); // green on
      chain_servo_state(true);
      break;
    }
    case State::STOP_AT_INTERMEDIATE: {
      Serial.println(">>> Current State: STOP_AT_INTERMEDIATE <<<");
      set_esc_speed(encoder_count, INTERMEDIATE_ENCODER_COUNT, Direction::STOP);
      buzzer_state(true);
      neopixel_state(true, false); // red on
      chain_servo_state(true);
      break;
    }
    case State::END: {
      Serial.println(">>> Current State: END <<<");
      set_esc_speed(encoder_count, MIN_ENCODER_COUNT, Direction::STOP);
      buzzer_state(false);
      neopixel_state(false, false);
      chain_servo_state(true);
      break;
    }
    default: {
      Serial.println("Error: Unknown state");
      break;
    }
  }
}
