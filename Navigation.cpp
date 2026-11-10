#include "Navigation.h"

#include <Arduino.h>
#include <cmath>

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

  stop();
  advancing = false;
  currentMode = Config::INICIAR_LIGADO ? MODE_AUTO : MODE_MANUAL;
  currentAction = (currentMode == MODE_AUTO) ? "Avancando em linha reta" : "Parado (Aguardando comando na interface)";
  return true;
}

const char* Navigation::getModeString() const {
  switch (currentMode) {
    case MODE_AUTO:
      return waypointActive ? "AUTO (WAYPOINT)" : "AUTO SLAM";
    case MODE_MANUAL:
      return "MANUAL";
    case MODE_RTH:
      return "RTH (RETORNO A BASE)";
    case MODE_ESTOP:
      return "E-STOP (EMERGENCIA)";
    default:
      return "DESCONHECIDO";
  }
}

int Navigation::getScannerAngle() const {
  return scanner.getCurrentAngle();
}

void Navigation::setMode(OperationalMode newMode) {
  if (currentMode == newMode) return;

  Serial.printf("[NAV] Alterando modo: %s -> ", getModeString());
  currentMode = newMode;
  Serial.println(getModeString());

  stop();
  advancing = false;

  if (currentMode == MODE_AUTO) {
    advanceStartedAt = millis();
    lastCellStepAt = millis();
    currentAction = "Auto SLAM ativado";
  } else if (currentMode == MODE_MANUAL) {
    waypointActive = false;
    currentAction = "Modo Manual (Aguardando comando)";
  } else if (currentMode == MODE_RTH) {
    targetWpX = 0.0f;
    targetWpY = 0.0f;
    waypointActive = true;
    currentAction = "Retornando a base (0, 0)...";
  } else if (currentMode == MODE_ESTOP) {
    currentAction = "EMERGENCIA: Motores travados!";
  }
}

void Navigation::setMode(const char* modeStr) {
  if (strcasecmp(modeStr, "auto") == 0) {
    setMode(MODE_AUTO);
  } else if (strcasecmp(modeStr, "manual") == 0) {
    setMode(MODE_MANUAL);
  } else if (strcasecmp(modeStr, "rth") == 0) {
    setMode(MODE_RTH);
  } else if (strcasecmp(modeStr, "estop") == 0) {
    emergencyStop();
  }
}

void Navigation::emergencyStop() {
  currentMode = MODE_ESTOP;
  stop();
  advancing = false;
  waypointActive = false;
  currentAction = "E-STOP ATIVADO! MOTORES TRAVADOS";
  Serial.println("\n[ALERT] ===============================================");
  Serial.println("[ALERT] >>> PARADA DE EMERGENCIA (E-STOP) DISPARADA <<<");
  Serial.println("[ALERT] ===============================================\n");
}

void Navigation::start() {
  setMode(MODE_AUTO);
}

void Navigation::pause() {
  setMode(MODE_MANUAL);
}

void Navigation::toggleRunning() {
  if (currentMode == MODE_AUTO || currentMode == MODE_RTH) {
    pause();
  } else {
    start();
  }
}

void Navigation::reset() {
  const OperationalMode prevMode = currentMode;
  stop();
  advancing = false;
  map.reset();
  reverseAttempts = 0;
  blockedCount = 0;
  obstacleBurst = 0;
  preferLeft = true;
  lastFrontCm = -1.0f;
  progressRefCm = -1.0f;
  waypointActive = false;
  scanner.position(Config::FRENTE);
  currentAction = "Mapa e percurso reiniciados";
  map.printMap();
  Serial.println("[NAV] Mapa e percurso reiniciados com sucesso!");

  if (prevMode == MODE_AUTO) {
    start();
  } else {
    setMode(prevMode);
  }
}

void Navigation::setSpeed(int speed) {
  cruiseSpeed = constrain(speed, 60, 255);
  Serial.printf("[NAV] Velocidade de cruzeiro atualizada: %d PWM (%.0f%%)\n",
                cruiseSpeed, (cruiseSpeed / 255.0f) * 100.0f);
  if (advancing && (currentMode == MODE_AUTO || currentMode == MODE_RTH)) {
    motors.move(1, 1, cruiseSpeed, cruiseSpeed);
  }
}

void Navigation::manualMove(const char* dir, int speed) {
  if (currentMode == MODE_ESTOP && strcmp(dir, "STOP") != 0) {
    Serial.println("[NAV] Comando manual ignorado: E-STOP ativo!");
    return;
  }

  currentMode = MODE_MANUAL;
  waypointActive = false;
  speed = constrain(speed, 60, 255);

  if (strcmp(dir, "STOP") == 0) {
    stop();
    advancing = false;
    currentAction = "MANUAL - Parado";
    pitch = 0.0f;
    roll = 0.0f;
    return;
  }

  int dirLeft = 0;
  int dirRight = 0;
  int speedLeft = 0;
  int speedRight = 0;

  // Garante torque ideal para curvas de pivô de roda única sem estol mecânico
  const int turnSpeed = max(speed, Config::VELOCIDADE_GIRO);

  if (strcmp(dir, "F") == 0) {
    // 1. Frente: Ambas as rodas giram para frente
    dirLeft = 1;
    dirRight = 1;
    speedLeft = speed;
    speedRight = speed;
    advancing = true;
    currentAction = "MANUAL - Avancando";
    pitch = -1.5f;
    roll = 0.0f;
  } else if (strcmp(dir, "B") == 0) {
    // 2. Tras: Ambas as rodas giram para tras com torque pleno maximo (255)
    dirLeft = -1;
    dirRight = -1;
    const int revSpeed = max(speed, (int)Config::VELOCIDADE_RE);
    speedLeft = revSpeed;
    speedRight = revSpeed;
    advancing = false;
    currentAction = "MANUAL - Recuando";
    pitch = 1.5f;
    roll = 0.0f;
  } else if (strcmp(dir, "L") == 0) {
    // 3. Esquerda: Gira SOMENTE a roda da DIREITA para frente (roda esquerda parada)
    dirLeft = 0;
    dirRight = 1;
    speedLeft = 0;
    speedRight = turnSpeed;
    advancing = false;
    currentAction = "MANUAL - Curva Esquerda (Roda Dir)";
    pitch = 0.0f;
    roll = -2.5f;
  } else if (strcmp(dir, "R") == 0) {
    // 4. Direita: Gira SOMENTE a roda da ESQUERDA para frente (roda direita parada)
    dirLeft = 1;
    dirRight = 0;
    speedLeft = turnSpeed;
    speedRight = 0;
    advancing = false;
    currentAction = "MANUAL - Curva Direita (Roda Esq)";
    pitch = 0.0f;
    roll = 2.5f;
  }

  // Mapeia comandos de roda para os motores físicos de acordo com Config::MOTOR_A_NA_ESQUERDA
  const int motA = Config::MOTOR_A_NA_ESQUERDA ? dirLeft : dirRight;
  const int motB = Config::MOTOR_A_NA_ESQUERDA ? dirRight : dirLeft;
  const int spdA = Config::MOTOR_A_NA_ESQUERDA ? speedLeft : speedRight;
  const int spdB = Config::MOTOR_A_NA_ESQUERDA ? speedRight : speedLeft;

  motors.move(motA, motB, spdA, spdB);
}

void Navigation::setWaypoint(float targetX, float targetY) {
  targetWpX = targetX;
  targetWpY = targetY;
  waypointActive = true;
  if (currentMode != MODE_RTH) {
    currentMode = MODE_AUTO;
  }
  Serial.printf("[NAV] Novo Waypoint definido: X=%.2f, Y=%.2f\n", targetX, targetY);
}

void Navigation::clearWaypoint() {
  waypointActive = false;
  if (currentMode == MODE_RTH) {
    currentMode = MODE_MANUAL;
  }
  Serial.println("[NAV] Waypoint cancelado.");
}

bool Navigation::safeDelay(unsigned long ms) {
  const unsigned long start = millis();
  while (millis() - start < ms) {
    if (yieldCallback) {
      yieldCallback();
    }
    if (currentMode == MODE_ESTOP || currentMode == MODE_MANUAL) {
      stop();
      return false;
    }
    delay(5);
  }
  return true;
}

void Navigation::updateAttitudeAndSensors() {
  yaw = (float)((map.heading() * 90) % 360);

  int backDir = (map.heading() + 2) % 4;
  int d = 1;
  while (d <= 6) {
    int bx = map.x();
    int by = map.y();
    if (backDir == 0) by -= d;
    else if (backDir == 1) bx += d;
    else if (backDir == 2) by += d;
    else bx -= d;

    if (map.cellAt(bx, by) == GridMap::OBSTACULO) break;
    d++;
  }
  lastBackCm = d * 30.0f;
}

float Navigation::getBatteryVoltage() const {
  if (Config::PINO_BATERIA != 255) {
    int raw = analogRead(Config::PINO_BATERIA);
    return (raw / 4095.0f) * 3.3f * 2.0f;
  }
  float pct = getBatteryPct();
  return 6.4f + (pct / 100.0f) * 2.0f;
}

float Navigation::getBatteryPct() const {
  if (Config::PINO_BATERIA != 255) {
    float v = getBatteryVoltage();
    float pct = ((v - 6.4f) / 2.0f) * 100.0f;
    return constrain(pct, 0.0f, 100.0f);
  }
  float elapsedMins = millis() / 60000.0f;
  float pct = 100.0f - (elapsedMins * 0.45f);
  return constrain(pct, 15.0f, 100.0f);
}

void Navigation::update() {
  updateAttitudeAndSensors();

  if (currentMode == MODE_ESTOP) {
    stop();
    advancing = false;
    pitch = 0.0f;
    roll = 0.0f;
    return;
  }

  if (currentMode == MODE_MANUAL) {
    const unsigned long now = millis();
    if (now - lastManualReadAt >= 100) {
      lastManualReadAt = now;
      lastFrontCm = frontReading(sensor.readOnce());
    }
    return;
  }

  if (currentMode == MODE_RTH || (currentMode == MODE_AUTO && waypointActive)) {
    updateWaypointNavigation();
    return;
  }

  if (currentMode == MODE_AUTO) {
    updateAutoNavigation();
  }
}

void Navigation::updateWaypointNavigation() {
  const int curX = map.x();
  const int curY = map.y();
  const int targetX = (int)round(targetWpX);
  const int targetY = (int)round(targetWpY);

  if (curX == targetX && curY == targetY) {
    stop();
    waypointActive = false;
    currentMode = MODE_MANUAL;
    currentAction = (currentMode == MODE_RTH) ? "RTH: Chegou a Base (0, 0)!" : "Waypoint alcancado!";
    Serial.println("[NAV] >>> DESTINO ALCANCADO COM SUCESSO! <<<");
    return;
  }

  const int dx = targetX - curX;
  const int dy = targetY - curY;

  int desiredHeading = 0;
  if (abs(dx) >= abs(dy)) {
    desiredHeading = (dx > 0) ? 1 : 3;
  } else {
    desiredHeading = (dy > 0) ? 2 : 0;
  }

  if (map.heading() != desiredHeading) {
    int diff = (desiredHeading - map.heading() + 4) % 4;
    if (diff == 1) {
      turn(false);
    } else if (diff == 3) {
      turn(true);
    } else {
      turn180(true);
    }
    return;
  }

  const float front = frontReading(sensor.readOnce());
  if (isBlocked(front)) {
    analyzeObstacle();
    return;
  }

  motors.move(1, 1, cruiseSpeed, cruiseSpeed);
  currentAction = "Avancando rumo ao Waypoint";
  pitch = -1.5f;

  if (millis() - lastCellStepAt >= Config::TEMPO_POR_CELULA_MS) {
    lastCellStepAt = millis();
    map.stepForward();
    map.printMap();
  }
}

void Navigation::updateAutoNavigation() {
  const float front = frontReading(sensor.readOnce());

  // Throttled logging para não travar o buffer serial em alta frequência
  static unsigned long lastLogAt = 0;
  if (isBlocked(front) || millis() - lastLogAt >= 250) {
    lastLogAt = millis();
    printDistance("Frente=", front);
    Serial.println();
  }

  // DETECÇÃO IMEDIATA DE OBSTÁCULO (ZERO DELAY)
  if (isBlocked(front)) {
    // 1. Corta imediatamente os motores no primeiro indício do sensor (sem delay!)
    stop();

    // 2. Confirmação instantânea (~25ms) com motores já parados para filtrar ruído espúrio
    const float confirm = frontReading(sensor.readOnce());
    if (isBlocked(confirm) || confirm == 0.0f) {
      Serial.println("[NAV] >>> OBSTACULO CONFIRMADO! INICIANDO MANOBRA <<<");
      blockedCount = 0;
      analyzeObstacle();
      return;
    } else {
      Serial.println("[NAV] Falso alarme (ruído filtrado), retomando...");
    }
  } else {
    blockedCount = 0;
  }

  int targetSpeed = cruiseSpeed;
  if (front >= 0 && front < Config::DISTANCIA_DESACELERACAO_CM) {
    targetSpeed = max(Config::VELOCIDADE_MINIMA, (int)(cruiseSpeed * 0.80f));
    currentAction = "Aproximacao suave (< 50cm)";
  } else {
    currentAction = "Avancando em velocidade de cruzeiro";
  }

  if (!advancing) {
    Serial.println("ANDANDO PARA FRENTE");
    advanceStartedAt = millis();
    lastCellStepAt = millis();
    advancing = true;
    progressRefCm = -1.0f;
    motors.move(1, 1, targetSpeed, targetSpeed);
    pitch = -1.5f;
  } else {
    motors.move(1, 1, targetSpeed, targetSpeed);
  }

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

  if (millis() - advanceStartedAt >= Config::TEMPO_LIVRE_PARA_RESET_MS) {
    obstacleBurst = 0;
  }
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
  if (rawDistanceCm < 0) {
    if (lastFrontCm >= 0 && lastFrontCm <= Config::PROTECAO_ECO_PERDIDO_CM &&
        (millis() - lastValidFrontAt <= Config::PROTECAO_ECO_JANELA_MS)) {
      return 0.0f;
    }
    return -1.0f;
  }
  lastFrontCm = rawDistanceCm;
  lastValidFrontAt = millis();
  return rawDistanceCm;
}

void Navigation::stop() {
  motors.stop();
  advancing = false;
  blockedCount = 0;
  pitch = 0.0f;
  roll = 0.0f;
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

void Navigation::rotate(bool left, unsigned long durationMs) {
  stop();
  safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
  recordTurn(left);

  // Pra esquerda: gira somente roda direita. Pra direita: gira somente roda esquerda.
  const int dirLeft = left ? 0 : 1;
  const int dirRight = left ? 1 : 0;
  const int spdLeft = left ? 0 : Config::VELOCIDADE_GIRO;
  const int spdRight = left ? Config::VELOCIDADE_GIRO : 0;

  const int motA = Config::MOTOR_A_NA_ESQUERDA ? dirLeft : dirRight;
  const int motB = Config::MOTOR_A_NA_ESQUERDA ? dirRight : dirLeft;
  const int spdA = Config::MOTOR_A_NA_ESQUERDA ? spdLeft : spdRight;
  const int spdB = Config::MOTOR_A_NA_ESQUERDA ? spdRight : spdLeft;

  motors.move(motA, motB, spdA, spdB);

  safeDelay(durationMs);
  stop();
  safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
}

bool Navigation::reverse() {
  currentAction = "Manobra de re (afastando)...";
  stop();

  if (reverseAttempts >= Config::MAX_RES_SEGUIDAS) {
    Serial.println("Limite de res atingido. Robo perdido/preso!");
    scan360AndRecover();
    reverseAttempts = 0;
    return false;
  }

  safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  pitch = 1.5f;
  if (!safeDelay(Config::TEMPO_RE_MS)) {
    stop();
    return false;
  }
  stop();
  ++reverseAttempts;
  return safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
}

void Navigation::turn(bool left) {
  currentAction = left ? "Girando para a Esquerda" : "Girando para a Direita";
  roll = left ? -3.0f : 3.0f;
  rotate(left, Config::TEMPO_GIRO_MS);

  if (left) {
    map.turnLeft();
  } else {
    map.turnRight();
  }

  preferLeft = !left;
  lastTurnAt = millis();
}

bool Navigation::turnUntilFree(bool left, int maxExtraTurns) {
  for (int turnCount = 0; turnCount <= maxExtraTurns; ++turnCount) {
    if (turnCount > 0) {
      Serial.print("Giro adicional #");
      Serial.println(turnCount);
    }
    turn(left);
    if (currentMode != MODE_AUTO && currentMode != MODE_RTH) return false;

    const float front = frontReading(sensor.readOnce());
    if (isFree(front)) {
      return true;
    }
  }
  return false;
}

void Navigation::turn180(bool turnLeft180) {
  currentAction = "Executando Meia-Volta (180 graus)";
  roll = turnLeft180 ? -3.0f : 3.0f;
  rotate(turnLeft180, Config::TEMPO_GIRO_180_MS);

  if (turnLeft180) {
    map.turnLeft();
    map.turnLeft();
  } else {
    map.turnRight();
    map.turnRight();
  }

  preferLeft = !turnLeft180;
  lastTurnAt = millis();
}

void Navigation::turnAround(bool left) {
  turn180(left);
}

void Navigation::escapeCorner() {
  stop();
  safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
  Serial.println("\n[PRESO EM QUINA] Recuando e buscando fuga...");

  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  pitch = 1.5f;
  if (!safeDelay(Config::TEMPO_RE_SEM_SAIDA_MS)) {
    stop();
    return;
  }
  stop();
  if (!safeDelay(Config::TEMPO_ESTABILIZACAO_MS)) return;

  const float left = scanner.look(Config::ESQUERDA, sensor);
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }
  const float right = scanner.look(Config::DIREITA, sensor);
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }
  scanner.position(Config::FRENTE);

  lastLeftCm = left > 0 ? left : 150.0f;
  lastRightCm = right > 0 ? right : 150.0f;

  const float spaceLeft = left < 0 ? 300.0f : left;
  const float spaceRight = right < 0 ? 300.0f : right;

  bool escapeDir = false;
  if (fabsf(spaceLeft - spaceRight) > 2.0f) {
    escapeDir = spaceLeft > spaceRight;
  } else {
    escapeLeft = !escapeLeft;
    escapeDir = escapeLeft;
  }

  turnAround(escapeDir);
  reverseAttempts = 0;
  obstacleBurst = 0;
  map.printMap();
}

void Navigation::scan360AndRecover() {
  Serial.println("[RECUPERACAO] Meia-volta para desobstruir...");
  preferLeft = !preferLeft;
  turn180(preferLeft);
}

void Navigation::recoverFromStuck() {
  stop();
  safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
  Serial.println("\n[SEM PROGRESSO] Objeto ou parede impedindo avanco.");

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

  motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
  pitch = 1.5f;
  if (!safeDelay(Config::TEMPO_RE_SEM_SAIDA_MS)) {
    stop();
    return;
  }
  stop();
  if (!safeDelay(Config::TEMPO_ESTABILIZACAO_MS)) return;

  const float left = scanner.look(Config::ESQUERDA, sensor);
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }
  const float right = scanner.look(Config::DIREITA, sensor);
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }
  scanner.position(Config::FRENTE);

  lastLeftCm = left > 0 ? left : 150.0f;
  lastRightCm = right > 0 ? right : 150.0f;

  const float spaceLeft = left < 0 ? 300.0f : left;
  const float spaceRight = right < 0 ? 300.0f : right;

  bool chooseLeft = false;
  if (fabsf(spaceLeft - spaceRight) > 2.0f) {
    chooseLeft = spaceLeft > spaceRight;
  } else {
    preferLeft = !preferLeft;
    chooseLeft = preferLeft;
  }

  if (spaceLeft > Config::LIMITE_CM || spaceRight > Config::LIMITE_CM) {
    turn(chooseLeft);
  } else {
    turn180(chooseLeft);
  }
  map.printMap();
}

void Navigation::analyzeObstacle() {
  currentAction = "Obstaculo! Analisando campo de visao...";
  map.markObstacle(map.heading());

  if (yieldCallback) yieldCallback();

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

  // Recuo adaptativo: garante afastamento suficiente antes do giro
  if (!reverse()) {
    return;
  }

  const int headingDeg = (map.heading() * 90) % 360;

  // 1. Varredura Lateral Esquerda (140 graus / -50 do centro)
  currentAction = "Varredura: Analisando Esquerda (140°)...";
  if (yieldCallback) yieldCallback();
  const float left = scanner.look(Config::ESQUERDA, sensor);
  lastLeftCm = left > 0 ? left : 150.0f;
  lastScannerDist = left;
  map.markRayAngle(headingDeg - 50.0f, left);
  if (yieldCallback) yieldCallback();
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }

  // 2. Varredura Diagonal Esquerda (115 graus / -25 do centro - detecta quina esquerda)
  currentAction = "Varredura: Diagonal Esquerda (115°)...";
  if (yieldCallback) yieldCallback();
  const float diagLeft = scanner.look(Config::DIAGONAL_E, sensor);
  map.markRayAngle(headingDeg - 25.0f, diagLeft);
  if (yieldCallback) yieldCallback();
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }

  // 3. Verificação Frontal pós-recuo (90 graus)
  currentAction = "Varredura: Frontal (90°)...";
  if (yieldCallback) yieldCallback();
  const float frontCheck = scanner.look(Config::FRENTE, sensor);
  map.markRayAngle(headingDeg, frontCheck);
  lastFrontCm = frontCheck;
  if (yieldCallback) yieldCallback();
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }

  // 4. Varredura Diagonal Direita (65 graus / +25 do centro - detecta quina direita)
  currentAction = "Varredura: Diagonal Direita (65°)...";
  if (yieldCallback) yieldCallback();
  const float diagRight = scanner.look(Config::DIAGONAL_D, sensor);
  map.markRayAngle(headingDeg + 25.0f, diagRight);
  if (yieldCallback) yieldCallback();
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }

  // 5. Varredura Lateral Direita (40 graus / +50 do centro)
  currentAction = "Varredura: Analisando Direita (40°)...";
  if (yieldCallback) yieldCallback();
  const float right = scanner.look(Config::DIREITA, sensor);
  lastRightCm = right > 0 ? right : 150.0f;
  lastScannerDist = right;
  map.markRayAngle(headingDeg + 50.0f, right);
  if (yieldCallback) yieldCallback();
  if (currentMode != MODE_AUTO && currentMode != MODE_RTH) { stop(); return; }

  // Retorna servo ao centro de forma suave
  scanner.positionSmooth(Config::FRENTE, 4);
  if (yieldCallback) yieldCallback();

  // DETECÇÃO DE QUINA / BECO EM "V" OU "U":
  // Se a frente e ambas as diagonais estiverem obstruídas, o robô está dentro de um canto!
  const bool trappedInCorner = (frontCheck > 0 && frontCheck < 35.0f) &&
                               (diagLeft > 0 && diagLeft < 38.0f) &&
                               (diagRight > 0 && diagRight < 38.0f);
  if (trappedInCorner) {
    Serial.println("\n[INTELIGÊNCIA SLAM] Encurralado em Quina/Beco! Executando fuga tática com ré estendida e meia-volta...");
    currentAction = "Preso em Quina! Fuga com meia-volta...";
    stop();
    safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
    motors.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
    safeDelay(Config::TEMPO_RE_SEM_SAIDA_MS);
    stop();
    safeDelay(Config::TEMPO_ESTABILIZACAO_MS);
    const bool escapeLeftDir = (left > right);
    turn180(escapeLeftDir);
    map.printMap();
    return;
  }

  // ALGORITMO DE SCORING DE CORREDOR INTELIGENTE:
  // Combina o espaço lateral amplo com a desobstrução da quina diagonal
  const float valLeft = left < 0 ? 300.0f : left;
  const float valDiagL = diagLeft < 0 ? 300.0f : diagLeft;
  const float valRight = right < 0 ? 300.0f : right;
  const float valDiagR = diagRight < 0 ? 300.0f : diagRight;

  float scoreL = (valLeft * 0.65f) + (valDiagL * 0.35f);
  float scoreR = (valRight * 0.65f) + (valDiagR * 0.35f);

  // Penalidade severa se a diagonal imediata do giro estiver colada em obstáculo
  if (valDiagL < Config::LIMITE_CM) scoreL *= 0.45f;
  if (valDiagR < Config::LIMITE_CM) scoreR *= 0.45f;

  // Adiciona inteligência de exploração e histórico do mapa SLAM
  scoreL += calculateDirectionScore(map.headingLeft(), scoreL);
  scoreR += calculateDirectionScore(map.headingRight(), scoreR);

  Serial.printf("[INTELIGÊNCIA SLAM] Escore de Saída: Esquerda=%.1f (Lat=%.1f, Diag=%.1f) vs Direita=%.1f (Lat=%.1f, Diag=%.1f)\n",
                scoreL, valLeft, valDiagL, scoreR, valRight, valDiagR);

  bool chooseLeft = false;
  if (isOscillating()) {
    preferLeft = !preferLeft;
    chooseLeft = preferLeft;
    Serial.println("[ANTI-OSCILAÇÃO] Forçando alternância de direção para quebrar ciclo.");
  } else if (fabsf(scoreL - scoreR) > 3.0f) {
    chooseLeft = (scoreL > scoreR);
  } else {
    preferLeft = !preferLeft;
    chooseLeft = preferLeft;
  }

  // Se ambos os lados estão fechados (< LIMITE_CM), executa meia-volta completa
  if (valLeft < Config::LIMITE_CM && valRight < Config::LIMITE_CM) {
    Serial.println("[NAV] Sem saída lateral viável. Executando meia-volta (180°)...");
    turn180(chooseLeft);
  } else {
    Serial.printf("[NAV] Decidido corredor: %s! Girando...\n", chooseLeft ? "ESQUERDA" : "DIREITA");
    turnUntilFree(chooseLeft, 1);
  }

  map.printMap();
}

void Navigation::recordTurn(bool left) {
  lastTurns[2] = lastTurns[1];
  lastTurns[1] = lastTurns[0];
  lastTurns[0] = left ? -1 : 1;

  lastTurnTimestamps[2] = lastTurnTimestamps[1];
  lastTurnTimestamps[1] = lastTurnTimestamps[0];
  lastTurnTimestamps[0] = millis();

  if (turnHistoryCount < 3) {
    ++turnHistoryCount;
  }
}

bool Navigation::isOscillating() const {
  if (turnHistoryCount < 3) return false;
  if (millis() - lastTurnTimestamps[2] > Config::JANELA_OSCILACAO_MS) return false;
  return (lastTurns[0] == -lastTurns[1]) && (lastTurns[1] == -lastTurns[2]);
}

float Navigation::calculateDirectionScore(int headingDir, float openSpaceCm) const {
  float score = openSpaceCm;
  const int unknown = map.unknownAhead(headingDir);
  const int visited = map.visitedAhead(headingDir);
  score += unknown * 18.0f;
  score -= visited * 25.0f;
  if (map.cellInDirection(headingDir) == GridMap::OBSTACULO) {
    score -= 150.0f;
  }
  return score;
}
