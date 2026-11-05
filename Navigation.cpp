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
  return distanceCm > (Config::LIMITE_CM + Config::HISTERESE_CM);
}

bool Navigation::isBlocked(float distanceCm) const {
  return distanceCm >= 0 && distanceCm <= Config::LIMITE_CM;
}

float Navigation::frontReading(float rawDistanceCm) {
  // Protecao contra colisao em pequenos obstaculos e obstaculos angulados:
  // Se o sensor estava lendo um obstaculo proximo recentemente e subitamente perde o eco,
  // pode ser objeto na zona cega ou que defletiu o sinal. Freia por precaucao, mas so para
  // objetos realmente proximos e leituras recentes (evita frear diante de espaco aberto).
  if (rawDistanceCm < 0) {
    if (lastValidFrontCm >= 0 && lastValidFrontCm <= Config::PROTECAO_ECO_PERDIDO_CM &&
        (millis() - lastValidFrontAt <= Config::PROTECAO_ECO_JANELA_MS)) {
      return 0.0f;
    }
    return -1.0f;
  }
  lastValidFrontCm = rawDistanceCm;
  lastValidFrontAt = millis();
  return rawDistanceCm;
}

void Navigation::stop() {
  motors.stop();
  advancing = false;
  blockedCount = 0;
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
    Serial.println("Limite de res atingido. Executando meia-volta (180) para desobstruir!");
    turn180(true);
    reverseAttempts = 0;
    return true;
  }

  ++reverseAttempts;
  Serial.println("RE: afastando suavemente do obstaculo.");
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_MS);

  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);
  return false;
}

void Navigation::rotate(bool left, unsigned long durationMs) {
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  const int leftMotor = left ? -1 : 1;
  const int rightMotor = -leftMotor;

  if (Config::MOTOR_A_NA_ESQUERDA) {
    motors.move(leftMotor, rightMotor, Config::VELOCIDADE_GIRO, Config::VELOCIDADE_GIRO);
  } else {
    motors.move(rightMotor, leftMotor, Config::VELOCIDADE_GIRO, Config::VELOCIDADE_GIRO);
  }

  delay(durationMs);
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);
}

void Navigation::recordTurn(bool left) {
  lastTurns[0] = lastTurns[1];
  lastTurns[1] = lastTurns[2];
  lastTurns[2] = left ? -1 : 1;

  lastTurnTimestamps[0] = lastTurnTimestamps[1];
  lastTurnTimestamps[1] = lastTurnTimestamps[2];
  lastTurnTimestamps[2] = millis();
  ++turnHistoryCount;
}

bool Navigation::isOscillating() const {
  if (turnHistoryCount < 3) {
    return false;
  }
  const bool pattern = (lastTurns[0] == -lastTurns[1]) && (lastTurns[1] == -lastTurns[2]);
  const bool recent = (millis() - lastTurnTimestamps[0] <= Config::JANELA_OSCILACAO_MS);
  return pattern && recent;
}

float Navigation::calculateDirectionScore(int headingDir, float openSpaceCm) const {
  const int unknown = map.unknownAhead(headingDir);
  const int visited = map.visitedAhead(headingDir);
  const float space = openSpaceCm < 0 ? 100.0f : (openSpaceCm > 100.0f ? 100.0f : openSpaceCm);

  // Heuristica do Grupo 4 (Exploracao com reducao de repeticao de trajeto):
  // +15 pts por celula desconhecida (exploracao de novas fronteiras)
  // +0.5 pt por cm de espaco livre (caminho seguro e aberto)
  // -12 pts por celula ja visitada (penaliza retorno a areas exploradas)
  return (unknown * 15.0f) + (space * 0.5f) - (visited * 12.0f);
}

void Navigation::turn(bool left) {
  Serial.println(left ? "GIRANDO ESQUERDA" : "GIRANDO DIREITA");
  recordTurn(left);
  rotate(left, Config::TEMPO_GIRO_MS);

  if (left) {
    map.turnLeft();
  } else {
    map.turnRight();
  }

  preferLeft = !left;
  lastTurnAt = millis();
}

void Navigation::turnAround(bool left) {
  // Meia-volta em UM giro continuo (mais preciso que dois giros com parada no meio)
  Serial.println(left ? "MEIA-VOLTA (180) PELA ESQUERDA" : "MEIA-VOLTA (180) PELA DIREITA");
  recordTurn(left);
  rotate(left, Config::TEMPO_GIRO_180_MS);

  if (left) {
    map.turnLeft();
    map.turnLeft();
  } else {
    map.turnRight();
    map.turnRight();
  }

  preferLeft = !left;
  lastTurnAt = millis();
}

void Navigation::analyzeObstacle() {
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  // Anti-Oscilacao: se o robo estiver preso em vaivem (ping-pong entre paredes),
  // executa recuo estendido e meia-volta para quebrar o ciclo
  if (isOscillating()) {
    Serial.println();
    Serial.println("[ANTI-OSCILACAO] Detectado vaivem repetitivo entre paredes!");
    Serial.println("Forcando fuga com recuo estendido e meia-volta (180)...");
    turnHistoryCount = 0;
    turn180(true);
    return;
  }

  // Deteccao de "preso": varios obstaculos em sequencia, sem conseguir andar
  if (millis() - obstacleWindowStart > Config::JANELA_PRESO_MS) {
    obstacleWindowStart = millis();
    obstacleBurst = 0;
  }
  ++obstacleBurst;
  if (obstacleBurst >= Config::OBSTACULOS_PARA_PRESO) {
    obstacleBurst = 0;
    obstacleWindowStart = millis();
    escapeCorner();
    return;
  }

  // 1. Verifica se o caminho a frente ja esta livre antes de qualquer manobra
  float front = sensor.readFiltered();
  if (isFree(front)) {
    Serial.println("[FRENTE LIVRE] Caminho desobstruido. Continuando em linha reta!");
    if (front < 0) {
      lastValidFrontCm = -1.0f;
    }
    if (obstacleBurst > 0) {
      --obstacleBurst;
    }
    return;
  }

  map.markRay(map.heading(), front);

  // 2. So da re se estiver muito proximo do obstaculo (zona critica <= MUITO_PERTO_CM)
  if (front >= 0 && front <= Config::MUITO_PERTO_CM) {
    Serial.println("Obstaculo muito proximo: dando RE para abrir espaco.");
    if (reverse()) {
      return;
    }
    front = sensor.readFiltered();
    if (isFree(front)) {
      Serial.println("[FRENTE LIVRE APOS RE] Espaco aberto. Continuando em linha reta!");
      return;
    }
  }

  // 3. Analisa os setores laterais com o servo rapidamente (Esquerda e Direita)
  Serial.println("Analisando setores laterais...");

  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);

  scanner.position(Config::FRENTE);
  front = sensor.readFiltered();

  printDistance("F=", front);
  printDistance("E=", left);
  printDistance("D=", right);
  Serial.println();

  // Mapeamento Raycast: projeta as distancias reais na grade
  map.markRay(map.heading(), front);
  map.markRay(map.headingLeft(), left);
  map.markRay(map.headingRight(), right);

  // 4. Se a frente liberou durante a verificacao, continua em linha reta
  if (isFree(front)) {
    Serial.println("[ESPACO LIVRE A FRENTE] Caminho livre detectado! Continuando em linha reta.");
    return;
  }

  // Espaco livre medido de cada lado (sem eco = espaco aberto amplo)
  const float spaceLeft = left < 0 ? 999.0f : left;
  const float spaceRight = right < 0 ? 999.0f : right;
  const bool leftFree = (left < 0) || (left > Config::LIMITE_CM);
  const bool rightFree = (right < 0) || (right > Config::LIMITE_CM);

  // Bateu em outro obstaculo logo depois de girar: da meia-volta para o lado oposto
  if (millis() - lastTurnAt < Config::JANELA_APOS_GIRO_MS) {
    Serial.println("Obstaculo logo apos girar: executando meia-volta (180).");
    turn180(spaceLeft >= spaceRight);
    return;
  }

  // 5. DECISAO RAPIDA DE SAIDA:
  // CASO 1: AMBOS OS LADOS BLOQUEADOS (QUINA DE PAREDE / BECO SEM SAIDA)
  // Em uma quina de 90 graus (como visto na foto), a frente e ambos os lados estao fechados.
  // A unica saida aberta e direta e atras: recua e gira 180 graus diretamente!
  if (!leftFree && !rightFree) {
    Serial.println();
    Serial.println("=============================================================");
    Serial.println("[QUINA DETECTADA] Frente e ambos os lados bloqueados!");
    Serial.println("Executando recuo e meia-volta (180) rapida para o espaco aberto...");
    Serial.println("=============================================================");
    turn180(spaceLeft >= spaceRight);
    return;
  }

  // CASO 2: PELO MENOS UM DOS LADOS ESTA LIVRE
  bool chooseLeft = false;

  if (leftFree && !rightFree) {
    chooseLeft = true;
  } else if (!leftFree && rightFree) {
    chooseLeft = false;
  } else {
    // Ambos os lados estao livres (> LIMITE_CM):
    // Usa a heuristica de mapa do Grupo 4 (fronteiras desconhecidas e espaco livre)
    const float scoreLeft = calculateDirectionScore(map.headingLeft(), spaceLeft);
    const float scoreRight = calculateDirectionScore(map.headingRight(), spaceRight);

    Serial.print("  [HEURISTICA] Score Esquerda: ");
    Serial.print(scoreLeft, 1);
    Serial.print(" | Score Direita: ");
    Serial.println(scoreRight, 1);

    if (fabsf(scoreLeft - scoreRight) > 1.5f) {
      chooseLeft = scoreLeft > scoreRight;
    } else {
      chooseLeft = spaceLeft >= spaceRight;
    }
  }

  Serial.print("Girando para o lado mais livre: ");
  Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
  turn(chooseLeft);
  map.printMap();
}

void Navigation::turn180(bool turnLeft180) {
  stop();
  Serial.println();
  Serial.println("=============================================================");
  Serial.println("[MEIA-VOLTA 180] Recuando e virando para o espaco aberto...");
  Serial.println("=============================================================");

  // 1) Recua centimetros a mais para descolar da quina/parede antes de girar
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_SEM_SAIDA_MS);
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  // 2) Meia-volta de 180 graus continua diretamente para a saida aberta
  turnAround(turnLeft180);
  scanner.position(Config::FRENTE);

  reverseAttempts = 0;
  obstacleBurst = 0;
  map.printMap();
}

bool Navigation::turnUntilFree(bool left, int maxExtraTurns) {
  (void)left;
  (void)maxExtraTurns;
  return true;
}

void Navigation::escapeCorner() {
  stop();
  Serial.println();
  Serial.println("=============================================================");
  Serial.println("[PRESO EM QUINA] Recuando e executando meia-volta rapida...");
  Serial.println("=============================================================");

  // 1) Recua centimetros a mais para se afastar da quina
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_SEM_SAIDA_MS);
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  // 2) Meia-volta para sair da quina
  const bool direction = escapeLeft;
  escapeLeft = !escapeLeft;
  turnAround(direction);
  scanner.position(Config::FRENTE);

  reverseAttempts = 0;
  obstacleBurst = 0;
  map.printMap();
}

void Navigation::scan360AndRecover() {
  Serial.println("[RECUPERACAO] Executando meia-volta direta para desobstruir...");
  turn180(true);
}

void Navigation::recoverFromStuck() {
  stop();
  Serial.println();
  Serial.println("[SEM PROGRESSO] Objeto ou parede impedindo avanco.");
  Serial.println("Recuando centimetros a mais e buscando a melhor opcao de saida...");

  // Conta como obstaculo para a deteccao de "preso"
  if (millis() - obstacleWindowStart > Config::JANELA_PRESO_MS) {
    obstacleWindowStart = millis();
    obstacleBurst = 0;
  }
  ++obstacleBurst;
  if (obstacleBurst >= Config::OBSTACULOS_PARA_PRESO) {
    obstacleBurst = 0;
    obstacleWindowStart = millis();
    escapeCorner();
    return;
  }

  // 1) Recua centimetros a mais para descolar da obstrucao
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_SEM_SAIDA_MS);
  stop();
  delay(Config::TEMPO_ESTABILIZACAO_MS);

  // 2) Escaneia rapidamente os lados para ver a melhor opcao de saida
  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);
  scanner.position(Config::FRENTE);

  const float spaceLeft = left < 0 ? 999.0f : left;
  const float spaceRight = right < 0 ? 999.0f : right;

  if (spaceLeft > Config::LIMITE_CM || spaceRight > Config::LIMITE_CM) {
    const bool chooseLeft = spaceLeft >= spaceRight;
    Serial.print("Melhor saida encontrada: ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
  } else {
    Serial.println("Laterais bloqueadas: executando meia-volta de 180 graus...");
    turn180(spaceLeft >= spaceRight);
  }
  map.printMap();
}

void Navigation::update() {
  const float front = frontReading(sensor.readOnce());
  printDistance("Frente=", front);
  Serial.println();

  if (isBlocked(front)) {
    ++blockedCount;
    // Exige 2 leituras consecutivas de obstaculo (~120ms) para confirmar bloqueio real.
    // Elimina frenagens falsas provocadas por ruidos acusticos transientes ou ecos perdidos.
    if (blockedCount >= 2) {
      blockedCount = 0;
      stop();
      analyzeObstacle();
      return;
    }
  } else {
    blockedCount = 0;
  }

  // Velocidade estavel em linha reta: elimina oscilacoes continuas que causam zigue-zague
  int targetSpeed = Config::VELOCIDADE_FRENTE;
  if (front >= 0 && front < Config::DISTANCIA_DESACELERACAO_CM) {
    targetSpeed = Config::VELOCIDADE_MINIMA;
  }

  if (!advancing) {
    Serial.println("ANDANDO PARA FRENTE");
    advanceStartedAt = millis();
    lastCellStepAt = millis();
    advancing = true;
    progressRefCm = -1.0f;
    motors.move(1, 1, targetSpeed, targetSpeed);
  } else {
    motors.move(1, 1, targetSpeed, targetSpeed);
  }

  // Deteccao de falta de progresso: se esta andando mas a distancia a frente nao muda,
  // algo esta impedindo (objeto que o sensor nao enxerga bem, roda presa, encostado em parede).
  if (front >= 0 && front < Config::DISTANCIA_MAX_PROGRESSO_CM) {
    if (progressRefCm < 0 || fabsf(front - progressRefCm) > Config::PROGRESSO_MIN_CM) {
      progressRefCm = front;
      progressRefAt = millis();
    } else if (millis() - progressRefAt >= Config::TEMPO_SEM_PROGRESSO_MS) {
      progressRefCm = -1.0f;
      recoverFromStuck();
      return;
    }
  } else {
    progressRefCm = -1.0f;
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

  // Andou de forma continua por tempo suficiente: nao esta preso
  if (millis() - advanceStartedAt >= Config::TEMPO_LIVRE_PARA_RESET_MS) {
    obstacleBurst = 0;
  }
}
