#include <Arduino.h>

constexpr int LED_PIN = 48;
constexpr unsigned long BLINK_PERIOD_MS = 1000;

unsigned long lastToggle = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
  Serial.printf("[LED] GPIO%d -> OFF\n", LED_PIN);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastToggle >= BLINK_PERIOD_MS) {
    lastToggle = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    Serial.printf("[LED] GPIO%d -> %s\n", LED_PIN, ledState ? "ON" : "OFF");
  }
}
