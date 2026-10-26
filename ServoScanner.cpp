#include "ServoScanner.h"

#include <Arduino.h>

#include "Config.h"
#include "DistanceSensor.h"

bool ServoScanner::begin() {
  head.setPeriodHertz(50);
  head.attach(Config::PINO_SERVO, 1000, 2000);
  return head.attached();
}

void ServoScanner::position(int angle) {
  const int command = Config::INVERTER_OLHAR ? 180 - angle : angle;
  head.write(command);
  delay(Config::TEMPO_SERVO_MS);
}

float ServoScanner::look(int angle, DistanceSensor& sensor) {
  position(angle);
  return sensor.readFiltered();
}
