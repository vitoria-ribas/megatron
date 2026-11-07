#pragma once

#include <ESP32Servo.h>

class DistanceSensor;

class ServoScanner {
 public:
  bool begin();
  void position(int angle);
  void positionSmooth(int targetAngle, int stepDelayMs = 4);
  float look(int angle, DistanceSensor& sensor);
  int getCurrentAngle() const { return currentAngle; }

 private:
  Servo head;
  int currentAngle = 90;
};
