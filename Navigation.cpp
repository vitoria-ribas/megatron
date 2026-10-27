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
  // Cabecalho das linhas STATS: copie-as do Monitor Serial para uma planilha.
  Serial.println(
      "STATS,tempo_ms,visitadas,revisitas,obstaculos,giros,res,"
      "desvios_repeticao");
  printStatus();
  return true;
}

void Navigation::printStatus() {
  map.printMap();

  Serial.print("STATS,");
  Serial.print(millis());
  Serial.print(',');
  Serial.print(map.visitedCount());
  Serial.print(',');
  Serial.print(map.revisitCount());
  Serial.print(',');
  Serial.print(map.obstacleCount());
  Serial.print(',');
  Serial.print(turnCount);
  Serial.print(',');
  Serial.print(reverseCount);
  Serial.print(',');
  Serial.println(repetitionTurnCount);
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

// O HC-SR04 perde o eco quando o objeto fica muito perto ou inclinado. Sem eco
// logo depois de ver algo perto na frente conta como muito perto, nao como livre.
float Navigation::frontReading(float distanceCm) {
  if (distanceCm >= 0) {
    lastValidFrontCm = distanceCm;
    lastValidFrontAt = millis();
    return distanceCm;
  }

  if (lastValidFrontCm >= 0 &&
      lastValidFrontCm <= Config::ECO_PERDIDO_PERTO_CM &&
      millis() - lastValidFrontAt <= Config::ECO_PERDIDO_JANELA_MS) {
    Serial.println("SEM ECO logo apos obstaculo perto: tratando como muito perto.");
    return 0.0f;
  }
  return distanceCm;
}

void Navigation::stop() {
  motors.stop();

  // Conta a celula em andamento se o robo ja percorreu pelo menos metade dela.
  if (advancing &&
      millis() - lastCellStepAt >= Config::TEMPO_POR_CELULA_MS / 2) {
    map.stepForward();
  }
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
  ++reverseCount;
  Serial.println("RE: afastando do obstaculo.");
  delay(Config::PAUSA_ANTES_MANOBRA_MS);

  motors.move(-1, -1);
  delay(Config::TEMPO_RE_MS);

  stop();
  delay(Config::PAUSA_APOS_MANOBRA_MS);
  return false;
}

void Navigation::turn(bool left) {
  stop();
  Serial.println(left ? "GIRANDO ESQUERDA" : "GIRANDO DIREITA");
  ++turnCount;
  lastValidFrontCm = -1.0f;  // a leitura antiga era de outra direcao
  delay(Config::PAUSA_ANTES_MANOBRA_MS);

  const int leftMotor = left ? -1 : 1;
  const int rightMotor = -leftMotor;

  if (Config::MOTOR_A_NA_ESQUERDA) {
    motors.move(leftMotor, rightMotor);
  } else {
    motors.move(rightMotor, leftMotor);
  }

  delay(Config::TEMPO_GIRO_MS);
  stop();
  delay(Config::PAUSA_APOS_MANOBRA_MS);

  if (left) {
    map.turnLeft();
  } else {
    map.turnRight();
  }

  preferLeft = !left;
}

void Navigation::scanSides(
    float& diagonalLeft, float& left, float& diagonalRight, float& right) {
  diagonalLeft = scanner.look(Config::DIAGONAL_E, sensor);
  left = scanner.look(Config::ESQUERDA, sensor);
  diagonalRight = scanner.look(Config::DIAGONAL_D, sensor);
  right = scanner.look(Config::DIREITA, sensor);
  scanner.position(Config::FRENTE);

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
}

// > 0 se a direcao a leva a uma area menos explorada que b; 0 em empate.
int Navigation::compareExploration(int a, int b) const {
  const int unknownDifference = map.unknownAhead(a) - map.unknownAhead(b);
  if (unknownDifference != 0 || !Config::EVITAR_REPETICAO) {
    return unknownDifference;
  }
  return map.visitsAhead(b) - map.visitsAhead(a);
}

void Navigation::avoidRepetition() {
  stop();
  Serial.println("Area ja visitada. Procurando caminho menos explorado...");
  delay(Config::PAUSA_ANTES_MANOBRA_MS);

  float diagonalLeft, left, diagonalRight, right;
  scanSides(diagonalLeft, left, diagonalRight, right);

  const int ahead = map.heading();
  const bool leftBetter = isFree(diagonalLeft) && isFree(left) &&
                          compareExploration(map.headingLeft(), ahead) > 0;
  const bool rightBetter = isFree(diagonalRight) && isFree(right) &&
                           compareExploration(map.headingRight(), ahead) > 0;

  if (!leftBetter && !rightBetter) {
    Serial.println("Nenhum lado menos explorado. Seguindo em frente.");
    return;
  }

  bool chooseLeft = leftBetter;
  if (leftBetter && rightBetter) {
    const int comparison =
        compareExploration(map.headingLeft(), map.headingRight());
    chooseLeft = comparison != 0 ? comparison > 0 : preferLeft;
  }

  ++repetitionTurnCount;
  Serial.print("Desviando para area menos explorada: ");
  Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
  turn(chooseLeft);
  printStatus();
}

void Navigation::analyzeObstacle() {
  stop();
  delay(Config::PAUSA_ANTES_MANOBRA_MS);

  float front = frontReading(sensor.readFiltered());
  if (isFree(front)) {
    Serial.println("Frente liberada.");
    return;
  }

  map.markObstacle(map.heading());

  // Muito perto: recua e depois varre os lados para escolher a direcao.
  bool reversed = false;
  if (front >= 0 && front <= Config::MUITO_PERTO_CM) {
    if (reverse()) {
      return;  // limite de res: ja girou para sair do beco
    }
    reversed = true;
  }

  Serial.println("Obstaculo confirmado. Analisando os lados...");

  float diagonalLeft, left, diagonalRight, right;
  scanSides(diagonalLeft, left, diagonalRight, right);

  front = frontReading(sensor.readFiltered());
  printDistance("F=", front);
  Serial.println();

  // Depois de uma re o obstaculo continua la: nao volta a seguir em frente.
  if (!reversed && isFree(front)) {
    Serial.println("Caminho frontal liberou durante a analise.");
    return;
  }

  if (!reversed && front >= 0 && front <= Config::MUITO_PERTO_CM) {
    if (reverse()) {
      return;
    }
  }

  const bool leftFree = isFree(diagonalLeft) && isFree(left);
  const bool rightFree = isFree(diagonalRight) && isFree(right);

  if (leftFree || rightFree) {
    bool chooseLeft = leftFree;

    if (leftFree && rightFree) {
      const int comparison =
          compareExploration(map.headingLeft(), map.headingRight());

      if (comparison != 0) {
        chooseLeft = comparison > 0;
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

        if (difference > -Config::MARGEM_EMPATE_CM &&
            difference < Config::MARGEM_EMPATE_CM) {
          chooseLeft = preferLeft;
        } else {
          chooseLeft = spaceLeft > spaceRight;
        }
      }
    }

    Serial.print("Escolhendo ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
    printStatus();
  } else {
    Serial.println("Frente e ambos os lados bloqueados: recuando e girando!");
    const bool alreadyTurned = reverse();
    if (!alreadyTurned) {
      turn(preferLeft);
    }
    printStatus();
  }
}

void Navigation::update() {
  const float front = frontReading(sensor.readOnce());
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
    ++cellsSinceRepetitionCheck;

    if (cellsSincePrint >= Config::IMPRIMIR_MAPA_A_CADA_N_CELULAS) {
      cellsSincePrint = 0;
      printStatus();
    }

    if (Config::EVITAR_REPETICAO &&
        map.visitsHere() >= Config::LIMITE_REVISITAS &&
        cellsSinceRepetitionCheck >= Config::CELULAS_ENTRE_VERIFICACOES) {
      cellsSinceRepetitionCheck = 0;
      avoidRepetition();
      return;
    }
  }

  if (millis() - advanceStartedAt >= Config::TEMPO_ZERAR_RES_MS) {
    reverseAttempts = 0;
  }
}
