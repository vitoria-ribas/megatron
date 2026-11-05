#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint8_t AIN1 = 14;
constexpr uint8_t AIN2 = 12;
constexpr uint8_t PWMA = 13;

constexpr uint8_t BIN1 = 27;
constexpr uint8_t BIN2 = 26;
constexpr uint8_t PWMB = 25;
constexpr uint8_t STBY = 33;

constexpr uint8_t TRIG = 19;
constexpr uint8_t ECHO = 21;
constexpr uint8_t PINO_SERVO = 2;

// Inversao de rotacao dos motores (ambos false para girarem sincronizados para frente)
constexpr bool INVERTER_A = false;
constexpr bool INVERTER_B = false;
constexpr bool MOTOR_A_NA_ESQUERDA = true;
constexpr bool INVERTER_OLHAR = false;

// Velocidade dos motores calibrada e estavel em linha reta (PWM via canais LEDC 4 e 5)
constexpr int VELOCIDADE_FRENTE = 150;   // Velocidade de cruzeiro firme e equilibrada (mantem linha reta)
constexpr int VELOCIDADE_MINIMA = 120;   // Velocidade de aproximacao final suave
constexpr int VELOCIDADE_GIRO = 135;     // Curva suave e precisa (~53%)
constexpr int VELOCIDADE_RE = 135;       // Recuo suave e controlado (~53%)
constexpr float DISTANCIA_DESACELERACAO_CM = 45.0f; // Reduz a marcha ao aproximar a menos de 45 cm
constexpr unsigned long JANELA_OSCILACAO_MS = 8000; // Janela para deteccao de armadilha de vaivem (ping-pong)

constexpr float FATOR_MOTOR_A = 1.0f;
constexpr float FATOR_MOTOR_B = 1.0f;

// Angulos do scanner
constexpr int FRENTE = 90;
constexpr int DIAGONAL_E = 120;
constexpr int ESQUERDA = 150;
constexpr int DIAGONAL_D = 60;
constexpr int DIREITA = 30;

// Limites de distancia
constexpr float LIMITE_CM = 32.0f;
constexpr float HISTERESE_CM = 6.0f;     // 32 + 6 = 38 cm para confirmar espaco livre
constexpr float MUITO_PERTO_CM = 15.0f;   // so recua se estiver a menos de 15 cm

constexpr float PROTECAO_ECO_PERDIDO_CM = 40.0f;
constexpr unsigned long PROTECAO_ECO_JANELA_MS = 400;

// Tempos de manobra e analise meticulosa (calibrados)
constexpr unsigned long TEMPO_ESTABILIZACAO_MS = 180;  // pausa para estabilizar o chassi antes de medir
constexpr unsigned long TEMPO_SERVO_MS = 300;          // tempo para o servo parar completamente no angulo
constexpr unsigned long TEMPO_RE_MS = 450;
constexpr unsigned long TEMPO_RE_SEM_SAIDA_MS = 750;   // recuo estendido ao ficar sem saida/em quina
constexpr unsigned long TEMPO_GIRO_MS = 400;           // giro suave de 90 graus
constexpr unsigned long TEMPO_GIRO_180_MS = 800;       // meia-volta suave de 180 graus
constexpr unsigned long JANELA_APOS_GIRO_MS = 3000;

constexpr int MAX_RES_SEGUIDAS = 2;

// Deteccao de falta de progresso
constexpr float PROGRESSO_MIN_CM = 3.0f;
constexpr unsigned long TEMPO_SEM_PROGRESSO_MS = 1800; // 1.8s sem variacao
constexpr float DISTANCIA_MAX_PROGRESSO_CM = 150.0f;

// Deteccao de "preso"
constexpr int OBSTACULOS_PARA_PRESO = 4;
constexpr unsigned long JANELA_PRESO_MS = 12000;
constexpr unsigned long TEMPO_LIVRE_PARA_RESET_MS = 2500;

constexpr bool SEM_ECO_LIVRE = true;
constexpr unsigned long TEMPO_POR_CELULA_MS = 600;     // avanco comedido no mapa
constexpr int IMPRIMIR_MAPA_A_CADA_N_CELULAS = 4;

}  // namespace Config
