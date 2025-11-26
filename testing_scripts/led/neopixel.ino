#include <Adafruit_NeoPixel.h>

// Pins
const int LED_PIN = 12;

// Consts
const int NUM_PIXELS = 5;

Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Colors
const uint32_t RED = pixels.Color(255, 0, 0);
const uint32_t BLUE = pixels.Color(0, 0, 255);
const uint32_t GREEN = pixels.Color(0, 255, 0);
const uint32_t YELLOW = pixels.Color(255, 150, 0);
const uint32_t OFF = pixels.Color(0, 0, 0);


void setup() {
  Serial.begin(9600);
  Serial.println("Neopixel test");

  pixels.begin();
  pixels.setBrightness(10);
  pixels.show();
}

void loop() {
  Serial.println("red");
  color_wipe(RED);
  delay(1000);

  Serial.println("green");
  color_wipe(GREEN);
  delay(1000);

  Serial.println("blue");
  color_wipe(BLUE);
  delay(1000);

  Serial.println("yellow");
  color_wipe(YELLOW);
  delay(1000);

  Serial.println("off");
  color_wipe(OFF);
  delay(1000);

  Serial.println("Rainbow");
  rainbow();
  delay(1000);

}

void color_wipe(uint32_t color) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, color);
    pixels.show();
  }
}

void rainbow() {
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < NUM_PIXELS; i++) {
      int pixelHue = firstPixelHue + (i * 65536L / NUM_PIXELS);
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
  }
  pixels.show();
}
