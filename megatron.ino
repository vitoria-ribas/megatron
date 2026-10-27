#include <Arduino.h>

#include "DistanceSensor.h"
#include "MotorController.h"
#include "Navigation.h"
#include "ServoScanner.h"

MotorController motores;
DistanceSensor sensor;
ServoScanner scanner;
Navigation navegacao(motores, sensor, scanner);

void setup() {
  Serial.begin(115200);

  if (!navegacao.begin()) {
    Serial.println("ERRO: servo nao configurado.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Robo Explorador pronto!");
  Serial.println("Iniciando em 3 segundos...");
  delay(3000);
}

void loop() {
  navegacao.update();
}
