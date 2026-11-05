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
  int blockedCount = 0;
  bool advancing = false;
  unsigned long advanceStartedAt = 0;
  unsigned long lastCellStepAt = 0;
  int cellsSincePrint = 0;
  bool preferLeft = true;
  float lastValidFrontCm = -1.0f;
  unsigned long lastValidFrontAt = 0;
  int obstacleBurst = 0;
  unsigned long obstacleWindowStart = 0;
  bool escapeLeft = true;
  unsigned long lastTurnAt = 0;
  float progressRefCm = -1.0f;
  unsigned long progressRefAt = 0;

  int lastTurns[3] = {0, 0, 0};
  unsigned long lastTurnTimestamps[3] = {0, 0, 0};
  int turnHistoryCount = 0;

  void recordTurn(bool left);
  bool isOscillating() const;
  float calculateDirectionScore(int headingDir, float openSpaceCm) const;

  void rotate(bool left, unsigned long durationMs);
  void turnAround(bool left);
  void recoverFromStuck();

  bool isFree(float distanceCm) const;
  bool isBlocked(float distanceCm) const;
  float frontReading(float rawDistanceCm);
  void stop();
  bool reverse();
  void turn(bool left);
  bool turnUntilFree(bool left, int maxExtraTurns);
  void turn180(bool turnLeft180);
  void escapeCorner();
  void scan360AndRecover();
  void analyzeObstacle();
  void printDistance(const char* label, float distanceCm);
};
