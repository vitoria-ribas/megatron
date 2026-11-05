#pragma once

class MotorController {
 public:
  void begin();
  void stop();
  void move(int motorA, int motorB);
  void move(int motorA, int motorB, int speedA, int speedB);

 private:
  int currentA = 0;
  int currentB = 0;
  int lastPwmA = -1;
  int lastPwmB = -1;
};
