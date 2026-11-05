#include "MotorController.h"

#include <Arduino.h>

#include "Config.h"

void MotorController::begin() {
  pinMode(Config::STBY, OUTPUT);
  pinMode(Config::AIN1, OUTPUT);
  pinMode(Config::AIN2, OUTPUT);
  pinMode(Config::BIN1, OUTPUT);
  pinMode(Config::BIN2, OUTPUT);

  // Anexa PWMA e PWMB nos canais LEDC 4 e 5 (Timer 1 em 1000 Hz, isolado do servo no Timer 0)
  ledcAttachChannel(Config::PWMA, 1000, 8, 4);
  ledcAttachChannel(Config::PWMB, 1000, 8, 5);

  digitalWrite(Config::STBY, HIGH);
  stop();
}

void MotorController::stop() {
  ledcWrite(Config::PWMA, 0);
  ledcWrite(Config::PWMB, 0);
  digitalWrite(Config::AIN1, LOW);
  digitalWrite(Config::AIN2, LOW);
  digitalWrite(Config::BIN1, LOW);
  digitalWrite(Config::BIN2, LOW);
  digitalWrite(Config::STBY, HIGH);
  currentA = 0;
  currentB = 0;
  lastPwmA = 0;
  lastPwmB = 0;
}

void MotorController::move(int motorA, int motorB) {
  move(motorA, motorB, Config::VELOCIDADE_FRENTE, Config::VELOCIDADE_FRENTE);
}

void MotorController::move(int motorA, int motorB, int speedA, int speedB) {
  digitalWrite(Config::STBY, HIGH);

  if (Config::INVERTER_A) {
    motorA = -motorA;
  }
  if (Config::INVERTER_B) {
    motorB = -motorB;
  }

  // 1. So comuta os pinos de direcao se o sentido dos motores realmente mudou
  // Evita chaveamento desnecessario da ponte H a cada ciclo (elimina micro-desvios e zigue-zague)
  if (currentA != motorA || currentB != motorB) {
    digitalWrite(Config::AIN1, motorA > 0 ? HIGH : LOW);
    digitalWrite(Config::AIN2, motorA < 0 ? HIGH : LOW);
    digitalWrite(Config::BIN1, motorB > 0 ? HIGH : LOW);
    digitalWrite(Config::BIN2, motorB < 0 ? HIGH : LOW);
    currentA = motorA;
    currentB = motorB;
    delayMicroseconds(50);
  }

  // 2. Calcula as velocidades com os fatores de sintonia
  int pwmA = 0;
  if (motorA != 0) {
    pwmA = constrain((int)(speedA * Config::FATOR_MOTOR_A), 0, 255);
  }

  int pwmB = 0;
  if (motorB != 0) {
    pwmB = constrain((int)(speedB * Config::FATOR_MOTOR_B), 0, 255);
  }

  // 3. So atualiza o PWM se o valor mudou (mantem o trem de pulso estavel sem jitter)
  if (lastPwmA != pwmA) {
    ledcWrite(Config::PWMA, pwmA);
    lastPwmA = pwmA;
  }
  if (lastPwmB != pwmB) {
    ledcWrite(Config::PWMB, pwmB);
    lastPwmB = pwmB;
  }
}
