#include "MotorController.h"

#include <Arduino.h>

#include "Config.h"

void MotorController::begin() {
  pinMode(Config::STBY, OUTPUT);
  pinMode(Config::AIN1, OUTPUT);
  pinMode(Config::AIN2, OUTPUT);
  pinMode(Config::PWMA, OUTPUT);
  pinMode(Config::BIN1, OUTPUT);
  pinMode(Config::BIN2, OUTPUT);
  pinMode(Config::PWMB, OUTPUT);
  stop();
}

void MotorController::stop() {
  if (Config::DEPURAR) {
    Serial.printf("[%lu] MOTOR PARADO\n", millis());
  }
  digitalWrite(Config::STBY, LOW);
  digitalWrite(Config::PWMA, LOW);
  digitalWrite(Config::PWMB, LOW);
}

void MotorController::move(int motorA, int motorB) {
  if (Config::DEPURAR) {
    Serial.printf("[%lu] MOTOR A=%d B=%d\n", millis(), motorA, motorB);
  }
  digitalWrite(Config::STBY, LOW);

  if (Config::INVERTER_A) {
    motorA = -motorA;
  }
  if (Config::INVERTER_B) {
    motorB = -motorB;
  }

  digitalWrite(Config::AIN1, motorA > 0 ? HIGH : LOW);
  digitalWrite(Config::AIN2, motorA < 0 ? HIGH : LOW);
  digitalWrite(Config::BIN1, motorB > 0 ? HIGH : LOW);
  digitalWrite(Config::BIN2, motorB < 0 ? HIGH : LOW);
  digitalWrite(Config::PWMA, motorA != 0 ? HIGH : LOW);
  digitalWrite(Config::PWMB, motorB != 0 ? HIGH : LOW);
  digitalWrite(Config::STBY, HIGH);
}
