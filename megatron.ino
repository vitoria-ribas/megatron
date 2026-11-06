#include <Arduino.h>

#include "DistanceSensor.h"
#include "MotorController.h"
#include "Navigation.h"
#include "ServoScanner.h"
#include "WebMap.h"

MotorController motores;
DistanceSensor sensor;
ServoScanner scanner;
Navigation navegacao(motores, sensor, scanner);
WebMap webMap;

void setup() {
  Serial.begin(115200);

  if (!navegacao.begin()) {
    Serial.println("ERRO: servo nao configurado.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Robo Explorador pronto!");
  webMap.begin(navegacao.getMap());
  Serial.println("Iniciando em 3 segundos...");
  delay(3000);
}

void loop() {
  navegacao.update();
  webMap.update();
}
