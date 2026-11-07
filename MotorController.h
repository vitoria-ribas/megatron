#pragma once

#include <Arduino.h>

class MotorController {
 public:
  void begin();
  void stop();
  void move(int motorA, int motorB);
  void move(int motorA, int motorB, int speedA, int speedB);
  void setTrim(float factorA, float factorB) { trimA = factorA; trimB = factorB; }
  float getTrimA() const { return trimA; }
  float getTrimB() const { return trimB; }

 private:
  int currentA = 0;
  int currentB = 0;
  int lastPwmA = -1;
  int lastPwmB = -1;
  float trimA = 1.0f;
  float trimB = 1.0f;
};
