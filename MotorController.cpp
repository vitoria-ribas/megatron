#include "MotorController.h"

#include <Arduino.h>

#include "Config.h"

void MotorController::begin() {
  trimA = Config::FATOR_MOTOR_A;
  trimB = Config::FATOR_MOTOR_B;

  pinMode(Config::STBY, OUTPUT);
  pinMode(Config::AIN1, OUTPUT);
  pinMode(Config::AIN2, OUTPUT);
  pinMode(Config::BIN1, OUTPUT);
  pinMode(Config::BIN2, OUTPUT);

  // Pinos candidatos adicionais para garantir acionamento de BIN2
  pinMode(32, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);

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

  digitalWrite(32, LOW);
  digitalWrite(4, LOW);
  digitalWrite(16, LOW);
  digitalWrite(17, LOW);
  digitalWrite(18, LOW);
  digitalWrite(22, LOW);
  digitalWrite(23, LOW);

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
  if (motorA == 0 && motorB == 0) {
    stop();
    return;
  }

  // Mantém ponte H sempre habilitada
  digitalWrite(Config::STBY, HIGH);

  if (Config::INVERTER_A) {
    motorA = -motorA;
  }
  if (Config::INVERTER_B) {
    motorB = -motorB;
  }

  // 1. Atualiza os pinos de direção para cada motor
  digitalWrite(Config::AIN1, motorA > 0 ? HIGH : LOW);
  digitalWrite(Config::AIN2, motorA < 0 ? HIGH : LOW);

  digitalWrite(Config::BIN1, motorB > 0 ? HIGH : LOW);
  const uint8_t valB2 = motorB < 0 ? HIGH : LOW;
  digitalWrite(Config::BIN2, valB2);
  digitalWrite(32, valB2);
  digitalWrite(4, valB2);
  digitalWrite(16, valB2);
  digitalWrite(17, valB2);
  digitalWrite(18, valB2);
  digitalWrite(22, valB2);
  digitalWrite(23, valB2);

  // 2. Calcula as velocidades individuais de cada motor
  int pwmA = 0;
  if (motorA != 0) {
    pwmA = constrain((int)(speedA * trimA), 0, 255);
  }

  int pwmB = 0;
  if (motorB != 0) {
    pwmB = constrain((int)(speedB * trimB), 0, 255);
  }

  // 3. Aplica o PWM via canais LEDC isolados 4 e 5
  ledcWrite(Config::PWMA, pwmA);
  ledcWrite(Config::PWMB, pwmB);

  currentA = motorA;
  currentB = motorB;
  lastPwmA = pwmA;
  lastPwmB = pwmB;
}
