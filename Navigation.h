#pragma once

#include <Arduino.h>
#include "Config.h"
#include "GridMap.h"

class DistanceSensor;
class MotorController;
class ServoScanner;

// ==============================================================================
// MODOS DE OPERAÇÃO DO MEGATRON
// ==============================================================================
enum OperationalMode {
  MODE_AUTO = 0,    // Mapeamento e navegação autônoma SLAM
  MODE_MANUAL,      // Controle manual por D-Pad / Teclas WASD
  MODE_RTH,         // Retorno autônomo à base (coordenada 0, 0)
  MODE_ESTOP        // Parada de emergência imediata (travamento de motores)
};

class Navigation {
 public:
  Navigation(
      MotorController& motors,
      DistanceSensor& sensor,
      ServoScanner& scanner);

  bool begin();
  void update();

  // Modos de Operação
  OperationalMode getMode() const { return currentMode; }
  const char* getModeString() const;
  void setMode(OperationalMode newMode);
  void setMode(const char* modeStr);
  void emergencyStop();

  // Controle de Execução (Ligar / Pausar)
  bool isRunning() const { return currentMode == MODE_AUTO || currentMode == MODE_RTH; }
  void start();   // Entra em MODE_AUTO e retoma movimento
  void pause();   // Entra em MODE_MANUAL e para motores imediatamente
  void toggleRunning();
  void reset();

  // Controle Manual (D-Pad / Teclado)
  void manualMove(const char* dir, int speed);

  // Navegação para Waypoint / RTH
  void setWaypoint(float targetX, float targetY);
  void clearWaypoint();
  bool hasActiveWaypoint() const { return waypointActive; }

  // Ajuste de Velocidade em Tempo Real
  void setSpeed(int speed);
  int getSpeed() const { return cruiseSpeed; }

  // Acesso à Telemetria e Sensores
  const GridMap& getMap() const { return map; }
  float getLastFrontCm() const { return lastFrontCm; }
  float getLastLeftCm() const { return lastLeftCm; }
  float getLastRightCm() const { return lastRightCm; }
  float getLastBackCm() const { return lastBackCm; }
  float getPitch() const { return pitch; }
  float getRoll() const { return roll; }
  float getYaw() const { return yaw; }
  float getBatteryPct() const;
  float getBatteryVoltage() const;
  const char* getActionName() const { return currentAction; }
  int getScannerAngle() const;
  float getLastScannerDist() const { return lastScannerDist; }

  typedef void (*YieldCallback)();
  void setYieldCallback(YieldCallback cb) { yieldCallback = cb; }
  bool safeDelay(unsigned long ms);

 private:
  MotorController& motors;
  DistanceSensor& sensor;
  ServoScanner& scanner;
  GridMap map;

  YieldCallback yieldCallback = nullptr;
  OperationalMode currentMode = MODE_AUTO;
  int cruiseSpeed = Config::VELOCIDADE_FRENTE;
  const char* currentAction = "Inicializando...";

  // Telemetria dinâmica e sensores
  float lastFrontCm = -1.0f;
  float lastLeftCm = 85.0f;
  float lastRightCm = 85.0f;
  float lastBackCm = 150.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  float yaw = 0.0f;
  unsigned long lastValidFrontAt = 0;
  unsigned long lastTelemetryUpdateAt = 0;
  unsigned long lastManualReadAt = 0;
  float lastScannerDist = -1.0f;

  // Waypoints e RTH
  bool waypointActive = false;
  float targetWpX = 0.0f;
  float targetWpY = 0.0f;

  // Variáveis de exploração autônoma
  int reverseAttempts = 0;
  int blockedCount = 0;
  bool advancing = false;
  unsigned long advanceStartedAt = 0;
  unsigned long lastCellStepAt = 0;
  int cellsSincePrint = 0;
  bool preferLeft = true;
  int obstacleBurst = 0;
  unsigned long obstacleWindowStart = 0;
  bool escapeLeft = true;
  unsigned long lastTurnAt = 0;
  float progressRefCm = -1.0f;
  unsigned long progressRefAt = 0;

  int lastTurns[3] = {0, 0, 0};
  unsigned long lastTurnTimestamps[3] = {0, 0, 0};
  int turnHistoryCount = 0;

  void updateAutoNavigation();
  void updateWaypointNavigation();
  void updateAttitudeAndSensors();

  void recordTurn(bool left);
  bool isOscillating() const;
  float calculateDirectionScore(int headingDir, float openSpaceCm) const;

  void rotate(bool left, unsigned long durationMs);
  void turnAround(bool left);
  void recoverFromStuck();

  bool isFree(float distanceCm) const;
  bool isBlocked(float distanceCm) const;
  float frontReading(float rawDistanceCm);
  void stop();
  bool reverse();
  void turn(bool left);
  bool turnUntilFree(bool left, int maxExtraTurns);
  void turn180(bool turnLeft180);
  void escapeCorner();
  void scan360AndRecover();
  void analyzeObstacle();
  void printDistance(const char* label, float distanceCm);
};
