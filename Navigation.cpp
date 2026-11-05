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
    Serial.println("Limite de res atingido. Robo perdido/preso!");
    scan360AndRecover();
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

  // 3. Analisa os setores laterais com o servo
  Serial.println("Analisando setores ao redor...");

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

  // Mapeamento Raycast: projeta as distancias reais na grade
  map.markRay(map.heading(), front);
  map.markRay(map.headingLeft(), left);
  map.markRay(map.headingRight(), right);

  // 4. REGRA FUNDAMENTAL: Se ao analisar o espaco frontal estiver livre,
  // CONTINUA EM LINHA RETA e NAO vira de posicao!
  if (isFree(front)) {
    Serial.println("[ESPACO LIVRE A FRENTE] Caminho livre detectado! Continuando em linha reta.");
    return;
  }

  // 5. Somente se a frente estiver BLOQUEADA, decide para onde virar:
  const bool leftFree = isFree(diagonalLeft) && isFree(left);
  const bool rightFree = isFree(diagonalRight) && isFree(right);

  // Espaco livre medido de cada lado (sem eco = muito espaco)
  const float dl = diagonalLeft < 0 ? 999.0f : diagonalLeft;
  const float el = left < 0 ? 999.0f : left;
  const float dr = diagonalRight < 0 ? 999.0f : diagonalRight;
  const float er = right < 0 ? 999.0f : right;
  const float openLeft = dl < el ? dl : el;
  const float openRight = dr < er ? dr : er;

  // Bateu em outro obstaculo logo depois de girar: da meia-volta para o lado oposto
  if (millis() - lastTurnAt < Config::JANELA_APOS_GIRO_MS) {
    Serial.println("Obstaculo logo apos girar: tentando o lado totalmente oposto (180).");
    turn180(openLeft >= openRight);
    return;
  }

  if (leftFree || rightFree) {
    bool chooseLeft = leftFree;

    if (leftFree && rightFree) {
      const float scoreLeft = calculateDirectionScore(map.headingLeft(), openLeft);
      const float scoreRight = calculateDirectionScore(map.headingRight(), openRight);

      Serial.print("  [HEURISTICA] Score Esquerda: ");
      Serial.print(scoreLeft, 1);
      Serial.print(" | Score Direita: ");
      Serial.println(scoreRight, 1);

      if (fabsf(scoreLeft - scoreRight) > 1.5f) {
        chooseLeft = scoreLeft > scoreRight;
      } else {
        chooseLeft = openLeft >= openRight;
      }
    }

    Serial.print("Frente bloqueada. Escolhendo melhor caminho: ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
    if (!turnUntilFree(chooseLeft, 1)) {
      Serial.println("Ainda sem saida frontal: varredura 360 para achar caminho...");
      scan360AndRecover();
    }
    map.printMap();
  } else {
    Serial.println("Frente e ambos os lados bloqueados: meia-volta de 180 graus...");
    turn180(openLeft >= openRight);
  }
}

void Navigation::turn180(bool turnLeft180) {
  stop();
  Serial.println();
  Serial.println("=============================================================");
  Serial.println("[SEM SAIDA] Frente e ambos os lados bloqueados!");
  Serial.println("Recuando centimetros a mais para descolar da quina/parede...");
  Serial.println("=============================================================");

  // 1) Recua centimetros a mais para descolar da quina antes de girar
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_SEM_SAIDA_MS);
  stop();
  delay(150);

  // 2) Meia-volta de 180 graus para o lado com mais espaco
  Serial.println("Girando 180 graus para buscar a saida oposta...");
  turnAround(turnLeft180);
  delay(150);

  // 3) Verifica a saida oposta
  scanner.position(Config::FRENTE);
  float opposite = frontReading(sensor.readFiltered());

  printDistance("Saida Oposta=", opposite);
  Serial.println();

  if (isFree(opposite)) {
    Serial.println("[MELHOR SAIDA ENCONTRADA] Caminho oposto totalmente livre! Prosseguindo...");
    reverseAttempts = 0;
    map.printMap();
    return;
  }

  // 4) Se o lado oposto ainda tiver algum obstaculo, analisa os lados para escolher a melhor saida
  Serial.println("Analisando setores ao redor para encontrar a melhor opcao de saida...");
  const float diagonalLeft = scanner.look(Config::DIAGONAL_E, sensor);
  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float diagonalRight = scanner.look(Config::DIAGONAL_D, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);
  scanner.position(Config::FRENTE);

  const float dl = diagonalLeft < 0 ? 999.0f : diagonalLeft;
  const float el = left < 0 ? 999.0f : left;
  const float dr = diagonalRight < 0 ? 999.0f : diagonalRight;
  const float er = right < 0 ? 999.0f : right;
  const float spaceLeft = dl < el ? dl : el;
  const float spaceRight = dr < er ? dr : er;

  if (isFree(diagonalLeft) && isFree(left)) {
    Serial.println("Melhor opcao de saida: ESQUERDA.");
    turn(true);
  } else if (isFree(diagonalRight) && isFree(right)) {
    Serial.println("Melhor opcao de saida: DIREITA.");
    turn(false);
  } else if (spaceLeft > spaceRight && spaceLeft > Config::LIMITE_CM) {
    Serial.println("Maior espaco livre: ESQUERDA.");
    turn(true);
  } else if (spaceRight > Config::LIMITE_CM) {
    Serial.println("Maior espaco livre: DIREITA.");
    turn(false);
  } else {
    Serial.println("Todas as direcoes proximas bloqueadas! Varredura 360 completa...");
    scan360AndRecover();
  }

  reverseAttempts = 0;
  map.printMap();
}

bool Navigation::turnUntilFree(bool left, int maxExtraTurns) {
  scanner.position(Config::FRENTE);
  float front = frontReading(sensor.readFiltered());
  int extra = 0;

  while (!isFree(front) && extra < maxExtraTurns) {
    Serial.println("Frente ainda bloqueada apos girar: continuando o giro...");
    turn(left);
    front = frontReading(sensor.readFiltered());
    ++extra;
  }

  return isFree(front);
}

void Navigation::escapeCorner() {
  stop();
  Serial.println();
  Serial.println("=============================================================");
  Serial.println("[PRESO] Muitos obstaculos seguidos: quina/corredor sem saida!");
  Serial.println("Recuando centimetros a mais e buscando a melhor saida...");
  Serial.println("=============================================================");

  // 1) Recua centimetros a mais para se afastar da quina
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  delay(Config::TEMPO_RE_SEM_SAIDA_MS);
  stop();
  delay(150);

  // 2) Meia-volta para sair da quina
  const bool direction = escapeLeft;
  escapeLeft = !escapeLeft;
  turnAround(direction);
  delay(150);

  // 3) Analisa a melhor saida apos a meia-volta
  scanner.position(Config::FRENTE);
  float front = frontReading(sensor.readFiltered());
  if (isFree(front)) {
    Serial.println("[SAIDA LIVRE] Caminho livre apos a meia-volta. Prosseguindo...");
    reverseAttempts = 0;
    map.printMap();
    return;
  }

  // Se nao liberou direto, analisa os lados para escolher a saida mais aberta
  const float diagonalLeft = scanner.look(Config::DIAGONAL_E, sensor);
  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float diagonalRight = scanner.look(Config::DIAGONAL_D, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);
  scanner.position(Config::FRENTE);

  const float dl = diagonalLeft < 0 ? 999.0f : diagonalLeft;
  const float el = left < 0 ? 999.0f : left;
  const float dr = diagonalRight < 0 ? 999.0f : diagonalRight;
  const float er = right < 0 ? 999.0f : right;
  const float spaceLeft = dl < el ? dl : el;
  const float spaceRight = dr < er ? dr : er;

  if (spaceLeft > Config::LIMITE_CM || spaceRight > Config::LIMITE_CM) {
    const bool chooseLeft = spaceLeft >= spaceRight;
    Serial.print("Melhor opcao de saida lateral: ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
    map.printMap();
  } else {
    Serial.println("[VARREDURA 360] Nenhuma saida lateral clara; buscando melhor rota...");
    scan360AndRecover();
  }
}

void Navigation::scan360AndRecover() {
  stop();
  Serial.println();
  Serial.println("=============================================================");
  Serial.println("[ALERTA] Robo perdido e sem espaco livre!");
  Serial.println("Iniciando VARREDURA 360 GRAUS para escanear o ambiente...");
  Serial.println("=============================================================");

  float distances[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
  int unknownScores[4] = {0, 0, 0, 0};
  bool freeDirs[4] = {false, false, false, false};

  scanner.position(Config::FRENTE);

  // Executa uma varredura 360 graus completa em 4 quadrantes cardeais (N, L, S, O)
  for (int step = 0; step < 4; ++step) {
    const int currentHeading = map.heading();
    delay(150);

    const float d = sensor.readFiltered();
    distances[currentHeading] = d;
    freeDirs[currentHeading] = isFree(d);
    unknownScores[currentHeading] = map.unknownAhead(currentHeading);

    if (isBlocked(d)) {
      map.markObstacle(currentHeading);
    }

    Serial.print("  [SCAN 360] Direcao ");
    Serial.print(GridMap::headingChar(currentHeading));
    Serial.print(" (");
    Serial.print(currentHeading);
    Serial.print("): Distancia = ");
    if (d >= 0) {
      Serial.print(d, 1);
      Serial.print(" cm");
    } else {
      Serial.print("SEM ECO (LIVRE)");
    }
    Serial.print(" | Celulas desconhecidas = ");
    Serial.print(unknownScores[currentHeading]);
    Serial.print(" | Status: ");
    Serial.println(freeDirs[currentHeading] ? "LIVRE" : "BLOQUEADO");

    // Gira 90 graus para escanear o proximo setor (o 4o setor ja fecha o circulo, sem girar a mais)
    if (step < 3) {
      turn(false);
    }
  }

  // Apos varrer os 4 setores, o robo esta no ultimo setor; o alinhamento abaixo usa o heading atual
  int bestHeading = -1;
  int maxUnknown = -1;
  float maxDist = -1.0f;

  // 1) Busca entre direcoes livres a com maior quantidade de celulas desconhecidas
  for (int h = 0; h < 4; ++h) {
    if (freeDirs[h]) {
      if (unknownScores[h] > maxUnknown) {
        maxUnknown = unknownScores[h];
        maxDist = distances[h] < 0 ? 999.0f : distances[h];
        bestHeading = h;
      } else if (unknownScores[h] == maxUnknown) {
        const float distH = distances[h] < 0 ? 999.0f : distances[h];
        if (distH > maxDist) {
          maxDist = distH;
          bestHeading = h;
        }
      }
    }
  }

  // 2) Se todas estiverem bloqueadas, escolhe a que tiver maior espaco livre
  if (bestHeading == -1) {
    float greatestDistance = -1.0f;
    for (int h = 0; h < 4; ++h) {
      const float distH = distances[h] < 0 ? 999.0f : distances[h];
      if (distH > greatestDistance) {
        greatestDistance = distH;
        bestHeading = h;
      }
    }
  }

  Serial.println("-------------------------------------------------------------");
  Serial.print("  [SCAN 360 CONCLUIDO] Direcao escolhida: ");
  Serial.print(GridMap::headingChar(bestHeading));
  Serial.println(". Alinhando o robo...");

  // Alinha o robo com a direcao escolhida
  const int currentHeading = map.heading();
  const int turnsToBest = (bestHeading - currentHeading + 4) % 4;

  if (turnsToBest == 1) {
    turn(false);  // 90 graus a direita
  } else if (turnsToBest == 2) {
    turnAround(false);  // 180 graus continuo
  } else if (turnsToBest == 3) {
    turn(true);   // 90 graus a esquerda
  }

  reverseAttempts = 0;
  Serial.println("  Robo alinhado com sucesso no caminho livre! Prosseguindo...");
  Serial.println("=============================================================");
  map.printMap();
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
  delay(150);

  // 2) Escaneia os lados para ver a melhor opcao de saida
  const float diagonalLeft = scanner.look(Config::DIAGONAL_E, sensor);
  const float left = scanner.look(Config::ESQUERDA, sensor);
  const float diagonalRight = scanner.look(Config::DIAGONAL_D, sensor);
  const float right = scanner.look(Config::DIREITA, sensor);
  scanner.position(Config::FRENTE);

  const float dl = diagonalLeft < 0 ? 999.0f : diagonalLeft;
  const float el = left < 0 ? 999.0f : left;
  const float dr = diagonalRight < 0 ? 999.0f : diagonalRight;
  const float er = right < 0 ? 999.0f : right;
  const float spaceLeft = dl < el ? dl : el;
  const float spaceRight = dr < er ? dr : er;

  if (spaceLeft > Config::LIMITE_CM || spaceRight > Config::LIMITE_CM) {
    const bool chooseLeft = spaceLeft >= spaceRight;
    Serial.print("Melhor saida encontrada: ");
    Serial.println(chooseLeft ? "ESQUERDA" : "DIREITA");
    turn(chooseLeft);
  } else {
    Serial.println("Laterais com pouco espaco: executando meia-volta de 180 graus...");
    turn180(spaceLeft >= spaceRight);
  }
  map.printMap();
}

void Navigation::update() {
  const float front = frontReading(sensor.readOnce());
  printDistance("Frente=", front);
  Serial.println();

  if (isBlocked(front)) {
    stop();
    analyzeObstacle();
    return;
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
