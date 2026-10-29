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
  unsigned long lastLogAt = 0;
  float lastValidFrontCm = -1.0f;
  unsigned long lastValidFrontAt = 0;

  // Metricas da exploracao, impressas nas linhas STATS.
  int turnCount = 0;
  int reverseCount = 0;
  bool preferLeft = true;

  bool isFree(float distanceCm) const;
  bool isBlocked(float distanceCm) const;
  float frontReading(float distanceCm);
  void stop();
  bool reverse();
  void turn(bool left);
  void scanSides(
      float& diagonalLeft, float& left, float& diagonalRight, float& right);
  int compareExploration(int a, int b) const;
  void analyzeObstacle();
  void printDistance(const char* label, float distanceCm);
  void printStatus();
};
