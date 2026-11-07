#include "DistanceSensor.h"

#include <Arduino.h>

#include "Config.h"

namespace {
constexpr unsigned long TRIGGER_INTERVAL_MS = 25; // Amostragem rápida a 40 Hz (25ms)
constexpr unsigned long ECHO_TIMEOUT_US = 18000UL; // 18ms = alcance max de ~3.1m (sem travas longas no vazio)
constexpr int FILTER_SAMPLES = 2;                  // 2 amostras ultra-rápidas para varredura ágil
}

void DistanceSensor::begin() {
  pinMode(Config::TRIG, OUTPUT);
  pinMode(Config::ECHO, INPUT);
  digitalWrite(Config::TRIG, LOW);
}

float DistanceSensor::readOnce() {
  const unsigned long interval = millis() - lastTriggerMs;
  if (interval < TRIGGER_INTERVAL_MS) {
    delay(TRIGGER_INTERVAL_MS - interval);
  }

  lastTriggerMs = millis();

  digitalWrite(Config::TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(Config::TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(Config::TRIG, LOW);

  const unsigned long pulse = pulseIn(Config::ECHO, HIGH, ECHO_TIMEOUT_US);
  if (pulse == 0) {
    return -1.0f;
  }

  const float distanceCm = pulse * 0.0343f / 2.0f;
  if (distanceCm < 2.0f) {
    return 0.0f;
  }
  if (distanceCm > 400.0f) {
    return -1.0f;
  }

  return distanceCm;
}

float DistanceSensor::readFiltered() {
  float values[3];
  int validCount = 0;

  for (int i = 0; i < 3; ++i) {
    const float d = readOnce();
    if (d >= 0) {
      values[validCount++] = d;
    }
  }

  if (validCount == 0) return -1.0f;
  if (validCount == 1) return values[0];
  if (validCount == 2) return (values[0] < values[1]) ? values[0] : values[1];

  // Mediana das 3 amostras para filtragem robusta contra reflexões
  if (values[0] > values[1]) { float t = values[0]; values[0] = values[1]; values[1] = t; }
  if (values[1] > values[2]) { float t = values[1]; values[1] = values[2]; values[2] = t; }
  if (values[0] > values[1]) { float t = values[0]; values[0] = values[1]; values[1] = t; }
  return values[1];
}
