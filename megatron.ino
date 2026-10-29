#include <Arduino.h>

#include "Config.h"
#include "DistanceSensor.h"
#include "MotorController.h"
#include "Navigation.h"
#include "ServoScanner.h"

MotorController motores;
DistanceSensor sensor;
ServoScanner scanner;
Navigation navegacao(motores, sensor, scanner);

void varrerServo() {
  const int angulos[] = {Config::FRENTE, Config::DIAGONAL_E, Config::ESQUERDA,
                         Config::DIAGONAL_D, Config::DIREITA, Config::FRENTE};
  for (const int angulo : angulos) {
    const float distancia = scanner.look(angulo, sensor);
    Serial.printf("  angulo %d: %.1f cm\n", angulo, distancia);
    delay(500);
  }
}

// Se o servo varre com os motores parados e trava com eles girando, a alimentacao
// do servo nao aguenta a queda de tensao dos motores.
void testarServo() {
  Serial.println("TESTE DO SERVO 1/2: motores parados");
  varrerServo();

  Serial.println("TESTE DO SERVO 2/2: motores girando (rodas no ar!)");
  motores.move(1, 1);
  varrerServo();
  motores.stop();

  Serial.println("Teste concluido. Ponha TESTE_SERVO = false e grave de novo.");
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);

  if (!navegacao.begin()) {
    Serial.println("ERRO: servo nao configurado.");
    while (true) {
      delay(1000);
    }
  }

  if (Config::TESTE_SERVO) {
    testarServo();
  }

  Serial.println("Robo Explorador pronto!");
  Serial.println("Iniciando em 3 segundos...");
  delay(3000);
}

void loop() {
  navegacao.update();
}
