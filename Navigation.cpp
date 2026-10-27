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

  map.begin();
  map.printMap();
  return true;
}

bool Navigation::isFree(float distanceCm) const {
  if (distanceCm < 0) {
    return Config::SEM_ECO_LIVRE;
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

bool Navigation::reverse() {
  stop();

  if (reverseAttempts >= Config::MAX_RES_SEGUIDAS) {
    Serial.println("Limite de res atingido. Girando para sair do beco...");
    turn(preferLeft);
    reverseAttempts = 0;
    return true;
  }

  ++reverseAttempts;
  Serial.println("RE: afastando do obstaculo.");
  delay(100);

  motors.move(-1, -1);
  delay(Config::TEMPO_RE_MS);

  stop();
  delay(150);
  return false;
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

  if (left) {
    map.turnLeft();
  } else {
    map.turnRight();
  }

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

  map.markObstacle(map.heading());

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

  if (isBlocked(left)) {
    map.markObstacle(map.headingLeft());
  }
  if (isBlocked(right)) {
    map.markObstacle(map.headingRight());
  }

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
      const int scoreLeft = map.unknownAhead(map.headingLeft());
      const int scoreRight = map.unknownAhead(map.headingRight());

      if (scoreLeft != scoreRight) {
        chooseLeft = scoreLeft > scoreRight;
      } else {
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
    }

    Serial.print("Escolhendo ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
    map.printMap();
  } else {
    Serial.println("Frente e ambos os lados bloqueados: recuando e girando!");
    const bool alreadyTurned = reverse();
    if (!alreadyTurned) {
      turn(preferLeft);
    }
    map.printMap();
  }
}

void Navigation::update() {
  const float front = sensor.readOnce();
  const bool blocked = isBlocked(front);

  // Imprime a leitura so periodicamente ou quando ha obstaculo, para o Serial ficar legivel.
  if (blocked || millis() - lastLogAt >= Config::INTERVALO_LOG_MS) {
    lastLogAt = millis();
    printDistance("Frente=", front);
    Serial.println();
  }

  if (blocked) {
    stop();
    analyzeObstacle();
    return;
  }

  if (!advancing) {
    Serial.println("ANDANDO PARA FRENTE");
    advanceStartedAt = millis();
    lastCellStepAt = millis();
    advancing = true;
    motors.move(1, 1);
  }

  if (millis() - lastCellStepAt >= Config::TEMPO_POR_CELULA_MS) {
    lastCellStepAt = millis();
    map.stepForward();
    ++cellsSincePrint;

    if (cellsSincePrint >= Config::IMPRIMIR_MAPA_A_CADA_N_CELULAS) {
      cellsSincePrint = 0;
      map.printMap();
    }
  }

  if (millis() - advanceStartedAt >= 500) {
    reverseAttempts = 0;
  }
}
