#pragma once

#include <Arduino.h>

namespace Config {

// ==============================================================================
// MAPEAMENTO DE PINOS DE HARDWARE (ESP32 Dev Module)
// ==============================================================================
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

// Inversao de sentido dos motores (false para girarem sincronizados para frente)
constexpr bool INVERTER_A = false;
constexpr bool INVERTER_B = false;
constexpr bool MOTOR_A_NA_ESQUERDA = true;
constexpr bool INVERTER_OLHAR = false;

// ==============================================================================
// CONFIGURAÇÕES DE ANDAMENTO E VELOCIDADES (PWM 0-255)
// ==============================================================================
// Velocidades controladas via canais LEDC 4 e 5 (1000 Hz, sem colisao de timer com o servo)
constexpr int VELOCIDADE_FRENTE = 150;   // Velocidade de cruzeiro firme e equilibrada (mantem linha reta)
constexpr int VELOCIDADE_MINIMA = 120;   // Velocidade de aproximacao suave (< 45 cm do obstaculo)
constexpr int VELOCIDADE_GIRO = 135;     // Curva suave e precisa (~53% de PWM)
constexpr int VELOCIDADE_RE = 135;       // Recuo suave e controlado (~53% de PWM)

// ==============================================================================
// CALIBRAÇÃO DE LINHA RETA (COMPENSAÇÃO ENTRE MOTORES)
// Se o robo tender a puxar levemente para um lado em piso liso, ajuste aqui:
// - Puxando para a ESQUERDA: aumente FATOR_MOTOR_A (ex: 1.05) ou reduza FATOR_MOTOR_B (ex: 0.95)
// - Puxando para a DIREITA: aumente FATOR_MOTOR_B (ex: 1.05) ou reduza FATOR_MOTOR_A (ex: 0.95)
// ==============================================================================
constexpr float FATOR_MOTOR_A = 1.0f;    // Motor Esquerdo
constexpr float FATOR_MOTOR_B = 1.0f;    // Motor Direito

// ==============================================================================
// DISTÂNCIAS E LIMITES DO SENSOR ULTRASSÔNICO
// ==============================================================================
constexpr float DISTANCIA_DESACELERACAO_CM = 45.0f; // Reduz a marcha para VELOCIDADE_MINIMA antes de parar
constexpr float LIMITE_CM = 32.0f;                 // Distancia segura de parada para analise (32 cm)
constexpr float HISTERESE_CM = 6.0f;               // 32 + 6 = 38 cm para confirmar espaco livre e seguro
constexpr float MUITO_PERTO_CM = 15.0f;            // So executa recuo (re) se parar a menos de 15 cm

constexpr float PROTECAO_ECO_PERDIDO_CM = 40.0f;
constexpr unsigned long PROTECAO_ECO_JANELA_MS = 400;

// ==============================================================================
// TEMPOS DE MANOBRA E ESTABILIZAÇÃO (EM MILISSEGUNDOS)
// ==============================================================================
constexpr unsigned long TEMPO_ESTABILIZACAO_MS = 180;  // Pausa para estabilizar o chassi antes de medir
constexpr unsigned long TEMPO_SERVO_MS = 300;          // Tempo para o servo estabilizar no angulo
constexpr unsigned long TEMPO_GIRO_MS = 400;           // Duracao do giro de 90 graus
constexpr unsigned long TEMPO_GIRO_180_MS = 800;       // Duracao da meia-volta (180 graus continuo)
constexpr unsigned long TEMPO_RE_MS = 450;             // Duracao do recuo padrao
constexpr unsigned long TEMPO_RE_SEM_SAIDA_MS = 750;   // Recuo estendido ao ficar sem saida/em quina
constexpr unsigned long TEMPO_POR_CELULA_MS = 600;     // Tempo estimado de avanco por celula no mapa

// ==============================================================================
// ÂNGULOS DO SCANNER SERVO (SG90)
// ==============================================================================
constexpr int FRENTE = 90;
constexpr int DIAGONAL_E = 120;
constexpr int ESQUERDA = 150;
constexpr int DIAGONAL_D = 60;
constexpr int DIREITA = 30;

// ==============================================================================
// PARÂMETROS DE CONTROLE DE NAVEGAÇÃO E ANTI-TRAVAMENTO
// ==============================================================================
constexpr unsigned long JANELA_OSCILACAO_MS = 8000;    // Janela para deteccao de ping-pong entre paredes
constexpr unsigned long JANELA_APOS_GIRO_MS = 3000;
constexpr int MAX_RES_SEGUIDAS = 2;

// Deteccao de falta de progresso (roda patinando ou obstaculo baixo)
constexpr float PROGRESSO_MIN_CM = 3.0f;
constexpr unsigned long TEMPO_SEM_PROGRESSO_MS = 1800; // 1.8s sem variacao
constexpr float DISTANCIA_MAX_PROGRESSO_CM = 150.0f;

// Deteccao de robo preso em quinas ou labirintos estreitos
constexpr int OBSTACULOS_PARA_PRESO = 4;
constexpr unsigned long JANELA_PRESO_MS = 12000;
constexpr unsigned long TEMPO_LIVRE_PARA_RESET_MS = 2500;

// Mapeamento em grade
constexpr bool SEM_ECO_LIVRE = true;
constexpr int IMPRIMIR_MAPA_A_CADA_N_CELULAS = 4;

}  // namespace Config
