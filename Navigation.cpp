#include "Navigation.h"

#include <Arduino.h>

#include "Config.h"
#include "DistanceSensor.h"
#include "MotorController.h"
#include "ServoScanner.h"

Navigation::Navigation(
    MotorController& motorController,
    DistanceSensor& distanceSensor,
    ServoScanner& servoScanner)
    : motors(motorController), sensor(distanceSensor), scanner(servoScanner) {}

bool Navigation::begin() {
  motors.begin();
  sensor.begin();

  if (!scanner.begin()) {
    return false;
  }

  scanner.position(Config::FRENTE);
  return true;
}

bool Navigation::isFree(float distanceCm) const {
  if (distanceCm < 0) {
    return true;
  }
  return distanceCm > Config::LIMITE_CM;
}

bool Navigation::isBlocked(float distanceCm) const {
  return distanceCm >= 0 && distanceCm <= Config::LIMITE_CM;
}

void Navigation::stop() {
  motors.stop();
  advancing = false;
}

void Navigation::printDistance(const char* label, float distanceCm) {
  Serial.print(label);
  if (distanceCm >= 0) {
    Serial.print(distanceCm, 1);
    Serial.print("cm");
  } else {
    Serial.print("LIVRE (SEM ECO)");
  }
  Serial.print(" | ");
}

void Navigation::reverse() {
  stop();

  if (reverseAttempts >= Config::MAX_RES_SEGUIDAS) {
    Serial.println("Limite de res atingido. Girando para sair do beco...");
    turn(preferLeft);
    delay(Config::TEMPO_GIRO_MS);
    reverseAttempts = 0;
    return;
  }

  ++reverseAttempts;
  Serial.println("RE: afastando do obstaculo.");
  delay(100);

  motors.move(-1, -1);
  delay(Config::TEMPO_RE_MS);

  stop();
  delay(150);
}

void Navigation::turn(bool left) {
  stop();
  Serial.println(left ? "GIRANDO ESQUERDA" : "GIRANDO DIREITA");
  delay(100);

  const int leftMotor = left ? -1 : 1;
  const int rightMotor = -leftMotor;

  if (Config::MOTOR_A_NA_ESQUERDA) {
    motors.move(leftMotor, rightMotor);
  } else {
    motors.move(rightMotor, leftMotor);
  }

  delay(Config::TEMPO_GIRO_MS);
  stop();
  delay(150);

  preferLeft = !left;
}

void Navigation::analyzeObstacle() {
  stop();
  delay(100);

  float front = sensor.readFiltered();
  if (isFree(front)) {
    Serial.println("Frente liberada.");
    return;
  }

  if (front >= 0 && front <= Config::MUITO_PERTO_CM) {
    reverse();
    return;
  }

  Serial.println("Obstaculo confirmado. Analisando os lados...");

  const float diagonalLeft = scanner.look(Config::DIAGONAL_E, sensor);
  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float diagonalRight = scanner.look(Config::DIAGONAL_D, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);

  scanner.position(Config::FRENTE);
  front = sensor.readFiltered();

  printDistance("F=", front);
  printDistance("DE=", diagonalLeft);
  printDistance("E=", left);
  printDistance("DD=", diagonalRight);
  printDistance("D=", right);
  Serial.println();

  if (isFree(front)) {
    Serial.println("Caminho frontal liberou durante a analise.");
    return;
  }

  if (front >= 0 && front <= Config::MUITO_PERTO_CM) {
    reverse();
    return;
  }

  const bool leftFree = isFree(diagonalLeft) && isFree(left);
  const bool rightFree = isFree(diagonalRight) && isFree(right);

  if (leftFree || rightFree) {
    bool chooseLeft = leftFree;

    if (leftFree && rightFree) {
      const float measuredDiagonalLeft =
          diagonalLeft < 0 ? 999.0f : diagonalLeft;
      const float measuredLeft = left < 0 ? 999.0f : left;
      const float measuredDiagonalRight =
          diagonalRight < 0 ? 999.0f : diagonalRight;
      const float measuredRight = right < 0 ? 999.0f : right;

      const float spaceLeft =
          measuredDiagonalLeft < measuredLeft
              ? measuredDiagonalLeft
              : measuredLeft;
      const float spaceRight =
          measuredDiagonalRight < measuredRight
              ? measuredDiagonalRight
              : measuredRight;
      const float difference = spaceLeft - spaceRight;

      if (difference > -5 && difference < 5) {
        chooseLeft = preferLeft;
      } else {
        chooseLeft = spaceLeft > spaceRight;
      }
    }

    turn(chooseLeft);
  } else {
    Serial.println("Frente e ambos os lados bloqueados: recuando e girando!");
    reverse();
    turn(preferLeft);
  }
}

void Navigation::update() {
  const float front = sensor.readOnce();
  printDistance("Frente=", front);
  Serial.println();

  if (isBlocked(front)) {
    stop();
    analyzeObstacle();
    return;
  }

  if (!advancing) {
    Serial.println("ANDANDO PARA FRENTE");
    advanceStartedAt = millis();
    advancing = true;
    motors.move(1, 1);
  }

  if (millis() - advanceStartedAt >= 500) {
    reverseAttempts = 0;
  }
}
