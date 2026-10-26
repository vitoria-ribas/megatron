#pragma once

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

  int reverseAttempts = 0;
  bool advancing = false;
  unsigned long advanceStartedAt = 0;
  bool preferLeft = true;

  bool isFree(float distanceCm) const;
  bool isBlocked(float distanceCm) const;
  void stop();
  void reverse();
  void turn(bool left);
  void analyzeObstacle();
  void printDistance(const char* label, float distanceCm);
};
