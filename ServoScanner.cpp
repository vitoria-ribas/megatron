#include "ServoScanner.h"

#include <Arduino.h>

#include "Config.h"
#include "DistanceSensor.h"

bool ServoScanner::begin() {
  // Aloca EXCLUSIVAMENTE o Timer 0 para o controle PWM de 50 Hz do servo
  // Deixa os Timers 1, 2 e 3 livres para os motores (LEDC Canais 4 e 5 no Timer 2)
  ESP32PWM::allocateTimer(0);
  head.setPeriodHertz(50);

  // Faixa de pulso calibrada para micro servo SG90 (500us a 2400us)
  head.attach(Config::PINO_SERVO, 500, 2400);

  position(Config::FRENTE);
  return head.attached();
}

void ServoScanner::position(int angle) {
  // Limita ângulo seguro para evitar bater nos batentes mecânicos de plástico do SG90
  angle = constrain(angle, 20, 160);
  currentAngle = angle;
  const int command = Config::INVERTER_OLHAR ? 180 - angle : angle;
  head.write(command);
  delay(Config::TEMPO_SERVO_MS);
}

void ServoScanner::positionSmooth(int targetAngle, int stepDelayMs) {
  targetAngle = constrain(targetAngle, 20, 160);
  if (targetAngle == currentAngle) return;

  const int dir = (targetAngle > currentAngle) ? 1 : -1;
  const int step = 2; // Passos suaves de 2 graus para transição fluida

  while (abs(targetAngle - currentAngle) > step) {
    currentAngle += dir * step;
    const int command = Config::INVERTER_OLHAR ? 180 - currentAngle : currentAngle;
    head.write(command);
    delay(stepDelayMs);
  }

  currentAngle = targetAngle;
  const int command = Config::INVERTER_OLHAR ? 180 - currentAngle : currentAngle;
  head.write(command);
  delay(Config::TEMPO_SERVO_MS); // Pausa deliberada no ângulo alvo para observação clara
}

float ServoScanner::look(int angle, DistanceSensor& sensor) {
  positionSmooth(angle, 4);
  return sensor.readFiltered();
}
