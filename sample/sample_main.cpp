#include <Arduino.h>

#define LED 2

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  digitalWrite(LED, HIGH);
  Serial.println("LED HIGH");
  delay(500);
  digitalWrite(LED, LOW);
  Serial.println("LED LOW");
  delay(500);
}