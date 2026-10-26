#pragma once

#include <ESP32Servo.h>

class DistanceSensor;

class ServoScanner {
 public:
  bool begin();
  void position(int angle);
  float look(int angle, DistanceSensor& sensor);

 private:
  Servo head;
};
