#include <Arduino.h>

#include "Config.h"
#include "DistanceSensor.h"
#include "MotorController.h"
#include "Navigation.h"
#include "ServoScanner.h"
#include "WebSocketManager.h"

// Componentes de Hardware e Controle
MotorController motores;
DistanceSensor sensor;
ServoScanner scanner;
Navigation navegacao(motores, sensor, scanner);
WebSocketManager wsManager;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=============================================================");
  Serial.println(">>> MEGATRON // AUTONOMOUS SLAM & TACTICAL HUD SYSTEM <<<");
  Serial.println("=============================================================");

  if (!navegacao.begin()) {
    Serial.println("[FATAL] Erro ao inicializar o servo scanner!");
    while (true) {
      delay(1000);
    }
  }

  // Permite que qualquer manobra demorada atenda o WebSocket e comandos imediatamente
  navegacao.setYieldCallback([]() {
    wsManager.update();
  });

  // Inicializa o Access Point Wi-Fi, Servidor HTTP Assíncrono e WebSocket
  if (!wsManager.begin(navegacao)) {
    Serial.println("[ERRO] Falha ao iniciar WebSocketManager!");
  }

  if (Config::INICIAR_LIGADO) {
    Serial.println("[NAV] Iniciando em modo AUTO SLAM em 2 segundos...");
    delay(2000);
    navegacao.start();
  } else {
    navegacao.pause();
    motores.stop();
    Serial.println("[NAV] Aguardando comando via HUD Web (http://192.168.4.1) ou Serial...");
  }
}

void loop() {
  // Comandos de emergência e depuração via Monitor Serial
  if (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == 's' || c == 'S' || c == '1') {
      navegacao.start();
    } else if (c == 'p' || c == 'P' || c == '0') {
      navegacao.pause();
    } else if (c == 'e' || c == 'E') {
      navegacao.emergencyStop();
    } else if (c == 'r' || c == 'R') {
      navegacao.reset();
    } else if (c == 'b' || c == 'B') {
      Serial.println("\n[TESTE EXCLUSIVO] Movendo SOMENTE a RODA ESQUERDA (MOTOR B) PARA TRAS por 6 segundos...");
      motores.move(0, -1, 0, Config::VELOCIDADE_RE);
      delay(6000);
      motores.stop();
      Serial.println("[TESTE EXCLUSIVO] Concluido: SOMENTE MOTOR B!\n");
    } else if (c == 'f' || c == 'F') {
      Serial.println("\n[TESTE] Movendo AMBAS as rodas para FRENTE por 4 segundos...");
      motores.move(1, 1, Config::VELOCIDADE_FRENTE, Config::VELOCIDADE_FRENTE);
      delay(4000);
      motores.stop();
      Serial.println("[TESTE] Concluido: AMBAS as rodas para FRENTE!\n");
    } else if (c == 'l' || c == 'L') {
      Serial.println("\n[TESTE] Girando ESQUERDA (Somente roda Direita para frente) por 4 segundos...");
      motores.move(0, 1, 0, Config::VELOCIDADE_GIRO);
      delay(4000);
      motores.stop();
      Serial.println("[TESTE] Concluido: ESQUERDA!\n");
    } else if (c == 'd' || c == 'D') {
      Serial.println("\n[TESTE] Girando DIREITA (Somente roda Esquerda para frente) por 4 segundos...");
      motores.move(1, 0, Config::VELOCIDADE_GIRO, 0);
      delay(4000);
      motores.stop();
      Serial.println("[TESTE] Concluido: DIREITA!\n");
    } else if (c == 't' || c == 'T') {
      Serial.println("\n[DIAGNOSTICO HARDWARE] Testando pinos e motores...");
      Serial.printf("Pinos: AIN1=%d, AIN2=%d (e 32), PWMA=%d | BIN1=%d, BIN2=%d, PWMB=%d | STBY=%d\n",
                    Config::AIN1, Config::AIN2, Config::PWMA, Config::BIN1, Config::BIN2, Config::PWMB, Config::STBY);
      
      Serial.println("\n--- FASE 1: Motor A (Re por 1.5s) ---");
      motores.move(-1, 0, Config::VELOCIDADE_RE, 0);
      delay(500);
      Serial.printf("Pinos lidos Motor A: AIN1=%d (esp 0), AIN2=%d (esp 1), GPIO32=%d (esp 1), STBY=%d (esp 1)\n",
                    digitalRead(Config::AIN1), digitalRead(Config::AIN2), digitalRead(32), digitalRead(Config::STBY));
      delay(1000);
      motores.stop();
      delay(400);

      Serial.println("\n--- FASE 2: Motor B (Re por 1.5s) ---");
      motores.move(0, -1, 0, Config::VELOCIDADE_RE);
      delay(500);
      Serial.printf("Pinos lidos Motor B: BIN1=%d (esp 0), BIN2=%d (esp 1), STBY=%d (esp 1)\n",
                    digitalRead(Config::BIN1), digitalRead(Config::BIN2), digitalRead(Config::STBY));
      delay(1000);
      motores.stop();
      delay(400);

      Serial.println("\n--- FASE 3: Ambos os Motores em Re simultanea (2s) ---");
      motores.move(-1, -1, Config::VELOCIDADE_RE, Config::VELOCIDADE_RE);
      delay(500);
      Serial.printf("Pinos lidos ambos: AIN2=%d, BIN2=%d, STBY=%d\n",
                    digitalRead(Config::AIN2), digitalRead(Config::BIN2), digitalRead(Config::STBY));
      delay(1500);
      motores.stop();
      Serial.println("[DIAGNOSTICO HARDWARE] Concluido com sucesso!\n");
    }
  }

  navegacao.update();
  wsManager.update();
}
