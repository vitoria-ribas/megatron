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

constexpr bool INVERTER_A = false;
constexpr bool INVERTER_B = false;
constexpr bool MOTOR_A_NA_ESQUERDA = true;
constexpr bool INVERTER_OLHAR = false;

constexpr int FRENTE = 90;
constexpr int DIAGONAL_E = 120;
constexpr int ESQUERDA = 150;
constexpr int DIAGONAL_D = 60;
constexpr int DIREITA = 30;

constexpr float LIMITE_CM = 25.0f;
constexpr float MUITO_PERTO_CM = 10.0f;

constexpr unsigned long TEMPO_SERVO_MS = 300;
constexpr unsigned long TEMPO_RE_MS = 350;
constexpr unsigned long TEMPO_GIRO_MS = 350;

constexpr int MAX_RES_SEGUIDAS = 2;
constexpr unsigned long TEMPO_ZERAR_RES_MS = 500;  // avanco minimo para zerar a contagem de res

constexpr unsigned long PAUSA_ANTES_MANOBRA_MS = 100;
constexpr unsigned long PAUSA_APOS_MANOBRA_MS = 150;
constexpr float MARGEM_EMPATE_CM = 5.0f;  // diferenca de espaco abaixo da qual os lados empatam

constexpr bool SEM_ECO_LIVRE = true;
// Sem eco ate ECO_PERDIDO_JANELA_MS depois de uma leitura frontal valida de ate
// ECO_PERDIDO_PERTO_CM conta como muito perto (o sensor perde o eco colado no objeto).
constexpr float ECO_PERDIDO_PERTO_CM = 50.0f;
constexpr unsigned long ECO_PERDIDO_JANELA_MS = 500;
constexpr unsigned long TEMPO_POR_CELULA_MS = 500;
constexpr int IMPRIMIR_MAPA_A_CADA_N_CELULAS = 4;
constexpr unsigned long INTERVALO_LOG_MS = 500;

// Anti-repeticao: ao entrar numa celula ja visitada, procura um lado menos explorado.
constexpr bool EVITAR_REPETICAO = true;  // false volta ao comportamento anterior
constexpr int LIMITE_REVISITAS = 2;  // passagens pela celula que disparam a verificacao
constexpr int CELULAS_ENTRE_VERIFICACOES = 3;  // evita parar a cada celula

}  // namespace Config
