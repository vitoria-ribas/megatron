#pragma once

#include <Arduino.h>

namespace Config {

// ==============================================================================
// 1. MAPEAMENTO DE PINOS DE HARDWARE (ESP32 Dev Module)
// ==============================================================================
// Driver Atual Padrão: TB6612FNG
// Motor A (Esquerdo): PWMA=13, AIN1=14, AIN2=12
// Motor B (Direito): PWMB=25, BIN1=27, BIN2=26
// Standby: STBY=33
constexpr uint8_t AIN1 = 14;
constexpr uint8_t AIN2 = 12;
constexpr uint8_t PWMA = 13;

constexpr uint8_t BIN1 = 27;
constexpr uint8_t BIN2 = 26;
constexpr uint8_t PWMB = 25;
constexpr uint8_t STBY = 33;

/* 
 * GUIA DE MAPEAMENTO PARA OUTRAS PONTES H:
 * 
 * 1) L298N:
 *    - ENA -> PWMA (13) [Controle PWM de Velocidade Motor Esquerdo]
 *    - IN1 -> AIN1 (14)
 *    - IN2 -> AIN2 (12)
 *    - IN3 -> BIN1 (27)
 *    - IN4 -> BIN2 (26)
 *    - ENB -> PWMB (25) [Controle PWM de Velocidade Motor Direito]
 *    - Deixar STBY livre ou conectado a 3.3V/5V.
 * 
 * 2) DRV8833:
 *    - AIN1 -> 14, AIN2 -> 12 (PWM direto ou comutação)
 *    - BIN1 -> 27, BIN2 -> 26
 *    - nSLEEP -> 33 (STBY em HIGH para habilitar a ponte)
 * 
 * 3) L9110S:
 *    - A-IA -> 14, A-IB -> 12
 *    - B-IA -> 27, B-IB -> 26
 */

// Sensores de Distância e Scanner
constexpr uint8_t TRIG = 19;        // HC-SR04 Trigger (ou pino XSHUT para VL53L0X)
constexpr uint8_t ECHO = 21;        // HC-SR04 Echo (ou I2C SDA para VL53L0X)
constexpr uint8_t PINO_SERVO = 2;    // Micro Servo SG90 (Varredura 120°/180°)

// Pino Opcional de Monitoramento de Bateria (Divisor Resistivo no ADC)
// Defina como 255 para estimativa automática via software (Li-ion 2S 7.4V - 8.4V)
constexpr uint8_t PINO_BATERIA = 255; // ex: 34, 35 ou 36 se tiver divisor resistivo

// Inversão de sentido dos motores (false para girarem sincronizados para frente)
constexpr bool INVERTER_A = false;
constexpr bool INVERTER_B = false;
constexpr bool MOTOR_A_NA_ESQUERDA = false;
constexpr bool INVERTER_OLHAR = false;

// ==============================================================================
// 2. CONFIGURAÇÕES DE ANDAMENTO E VELOCIDADES (PWM 0-255)
// ==============================================================================
// Velocidades controladas via canais LEDC 4 e 5 (1000 Hz, isolados do servo no Timer 0)
constexpr int VELOCIDADE_FRENTE = 180;   // Velocidade de cruzeiro ágil e equilibrada
constexpr int VELOCIDADE_MINIMA = 130;   // Velocidade de aproximação suave (< 50 cm do obstáculo)
constexpr int VELOCIDADE_GIRO = 200;     // Curva vigorosa em pivô de roda única
constexpr int VELOCIDADE_RE = 220;       // Torque alto e vigoroso em marcha à ré sem sobrecarga

// ==============================================================================
// 3. CALIBRAÇÃO DE LINHA RETA (COMPENSAÇÃO ENTRE MOTORES)
// ==============================================================================
// Se o robô tender a puxar levemente para um lado em piso liso, ajuste aqui:
// - Puxando para a ESQUERDA: aumente FATOR_MOTOR_A (ex: 1.05) ou reduza FATOR_MOTOR_B (ex: 0.95)
// - Puxando para a DIREITA: aumente FATOR_MOTOR_B (ex: 1.05) ou reduza FATOR_MOTOR_A (ex: 0.95)
constexpr float FATOR_MOTOR_A = 1.0f;    // Fator multiplicador Motor Esquerdo
constexpr float FATOR_MOTOR_B = 1.0f;    // Fator multiplicador Motor Direito

// ==============================================================================
// 4. DISTÂNCIAS E LIMITES DO SENSOR ULTRASSÔNICO
// ==============================================================================
constexpr float DISTANCIA_DESACELERACAO_CM = 50.0f; // Reduz a marcha para VELOCIDADE_MINIMA antes de parar
constexpr float LIMITE_CM = 30.0f;                 // Distância de detecção de obstáculo (~30 cm)
constexpr float HISTERESE_CM = 6.0f;               // 30 + 6 = 36 cm para confirmar espaço livre e seguro
constexpr float MUITO_PERTO_CM = 25.0f;            // Limite para manobra de afastamento (25 cm)

constexpr float PROTECAO_ECO_PERDIDO_CM = 45.0f;
constexpr unsigned long PROTECAO_ECO_JANELA_MS = 300;

// ==============================================================================
// 5. TEMPOS DE MANOBRA E ESTABILIZAÇÃO (EM MILISSEGUNDOS)
// ==============================================================================
constexpr unsigned long TEMPO_ESTABILIZACAO_MS = 100;  // Pausa para estabilizar o chassi antes de medir (100ms)
constexpr unsigned long TEMPO_SERVO_MS = 260;          // Pausa estável do servo nos lados para medição clara
constexpr unsigned long TEMPO_GIRO_MS = 340;           // Duração do giro de 90 graus com torque elevado
constexpr unsigned long TEMPO_GIRO_180_MS = 680;       // Duração da meia-volta (180 graus contínuo)
constexpr unsigned long TEMPO_RE_MS = 750;             // Duração da ré de ~25 cm
constexpr unsigned long TEMPO_RE_SEM_SAIDA_MS = 1000;  // Recuo estendido ao ficar sem saída/em quina
constexpr unsigned long TEMPO_POR_CELULA_MS = 450;     // Tempo estimado de avanço por célula no mapa

// ==============================================================================
// 6. ÂNGULOS DO SCANNER SERVO (SG90)
// ==============================================================================
// Varredura suave e precisa (evita chicoteamento mecânico e limites extremos)
constexpr int FRENTE = 90;
constexpr int ESQUERDA = 140;
constexpr int DIREITA = 40;
constexpr int DIAGONAL_E = 115;
constexpr int DIAGONAL_D = 65;

// ==============================================================================
// 7. PARÂMETROS DE CONTROLE DE NAVEGAÇÃO E ANTI-TRAVAMENTO
// ==============================================================================
constexpr unsigned long JANELA_OSCILACAO_MS = 8000;    // Janela para detecção de ping-pong entre paredes
constexpr unsigned long JANELA_APOS_GIRO_MS = 3000;
constexpr int MAX_RES_SEGUIDAS = 2;

// Detecção de falta de progresso (roda patinando ou obstáculo baixo)
constexpr float PROGRESSO_MIN_CM = 3.0f;
constexpr unsigned long TEMPO_SEM_PROGRESSO_MS = 1800; // 1.8s sem variação
constexpr float DISTANCIA_MAX_PROGRESSO_CM = 45.0f;    // Só monitora progresso se estiver a menos de 45cm

// Detecção de robô preso em quinas ou labirintos estreitos
constexpr int OBSTACULOS_PARA_PRESO = 4;
constexpr unsigned long JANELA_PRESO_MS = 12000;
constexpr unsigned long TEMPO_LIVRE_PARA_RESET_MS = 2500;

// Estado inicial ao ligar a placa
constexpr bool INICIAR_LIGADO = false; // Inicia parado em modo MANUAL seguro; usuario ativa AUTO SLAM quando desejar

// Mapeamento em grade
constexpr bool SEM_ECO_LIVRE = true;
constexpr int IMPRIMIR_MAPA_A_CADA_N_CELULAS = 4;

// ==============================================================================
// 8. CONFIGURAÇÕES DO SERVIDOR ASSÍNCRONO WI-FI & WEBSOCKET
// ==============================================================================
constexpr const char* WIFI_SSID = "Megatron";
constexpr const char* WIFI_PASS = "12345678";
constexpr uint16_t PORTA_WEB = 80;
constexpr unsigned long INTERVALO_TELEMETRIA_MS = 100; // 10 Hz (alta fluidez sem latência)

}  // namespace Config
