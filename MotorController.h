#pragma once

class MotorController {
 public:
  void begin();
  void stop();
  void move(int motorA, int motorB);
};
