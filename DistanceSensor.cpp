#include "DistanceSensor.h"

#include <Arduino.h>

#include "Config.h"

namespace {
constexpr unsigned long TRIGGER_INTERVAL_MS = 60;
constexpr unsigned long ECHO_TIMEOUT_US = 30000UL;
constexpr int FILTER_SAMPLES = 3;
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
  float values[FILTER_SAMPLES];
  int validCount = 0;
  int noEchoCount = 0;

  for (int i = 0; i < FILTER_SAMPLES; ++i) {
    const float distanceCm = readOnce();
    if (distanceCm < 0) {
      ++noEchoCount;
    } else {
      values[validCount++] = distanceCm;
    }
  }

  if (noEchoCount >= 2 || validCount == 0) {
    return -1.0f;
  }

  for (int i = 0; i < validCount - 1; ++i) {
    for (int j = i + 1; j < validCount; ++j) {
      if (values[j] < values[i]) {
        const float temporary = values[i];
        values[i] = values[j];
        values[j] = temporary;
      }
    }
  }

  return validCount == 3 ? values[1] : values[0];
}
