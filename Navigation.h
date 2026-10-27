#pragma once

#include "GridMap.h"

class DistanceSensor;
class MotorController;
class ServoScanner;

class Navigation {
 public:
  Navigation(
      MotorController& motors,
      DistanceSensor& sensor,
      ServoScanner& scanner);

  bool begin();
  void update();

 private:
  MotorController& motors;
  DistanceSensor& sensor;
  ServoScanner& scanner;
  GridMap map;

  int reverseAttempts = 0;
  bool advancing = false;
  unsigned long advanceStartedAt = 0;
  unsigned long lastCellStepAt = 0;
  int cellsSincePrint = 0;
  bool preferLeft = true;

  bool isFree(float distanceCm) const;
  bool isBlocked(float distanceCm) const;
  void stop();
  bool reverse();
  void turn(bool left);
  void analyzeObstacle();
  void printDistance(const char* label, float distanceCm);
};
