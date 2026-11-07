# MEGATRON // SISTEMA DE CONTROLE TÁTICO & TELEMETRIA SLAM ESP32

Sistema modernizado de telemetria, mapeamento SLAM e centro de comando tático para o robô ESP32 **MEGATRON**.

---

## 1. Arquitetura do Firmware e Organização dos Módulos

| Arquivo | Responsabilidade |
|---|---|
| `megatron.ino` | Ponto de entrada do firmware (`setup` e `loop`), orquestração do robô e tratamento do Serial. |
| `Config.h` | Mapeamento de pinos, parâmetros de calibração, velocidades PWM e tempos de manobra. |
| `WebUI.h` | Interface Web Cyberpunk / Sci-Fi HUD servida diretamente da memória Flash (`PROGMEM`). |
| `WebSocketManager.h/.cpp` | Ponto de acesso Wi-Fi, Servidor Web assíncrono (`ESPAsyncWebServer`) e WebSocket bidirecional (`AsyncWebSocket`). |
| `Navigation.h/.cpp` | Máquina de estados dos modos (Auto SLAM, Manual, RTH, E-STOP), controle de waypoints e evasão de obstáculos. |
| `GridMap.h/.cpp` | Mapeamento de ocupação em grade esparsa 2D, odometria relativa e histórico de trajetória. |
| `MotorController.h/.cpp` | Controle da ponte H (TB6612FNG, L298N, DRV8833 ou L9110S) via hardware PWM LEDC. |
| `DistanceSensor.h/.cpp` | Driver ultrassônico (HC-SR04) com filtragem de ruído. |
| `ServoScanner.h/.cpp` | Varredura angular com micro servo SG90 para cobertura de campo de visão (FOV). |

---

## 2. Bibliotecas Necessárias (Arduino IDE)

O firmware utiliza bibliotecas assíncronas de alto desempenho para garantir que a transmissão de telemetria e o recebimento de comandos ocorram sem **nenhum bloqueio de loop**:

1. **ESPAsyncWebServer** (mathieucarbou v3.x / compatível com ESP32 Core 3.x)
2. **AsyncTCP** (mathieucarbou v3.x)
3. **ArduinoJson** (v7.x)
4. **ESP32Servo** (v3.x)

*Todas as bibliotecas já se encontram instaladas no diretório local de bibliotecas do usuário.*

---

## 3. Pinagem e Ligações de Hardware (ESP32 Dev Module)

| Sinal | GPIO | Função |
|---|---:|---|
| **AIN1** | 14 | Direção A (Motor Esquerdo) |
| **AIN2** | 12 | Direção A (Motor Esquerdo) |
| **PWMA** | 13 | Velocidade PWM Motor Esquerdo (Canal LEDC 4) |
| **BIN1** | 27 | Direção B (Motor Direito) |
| **BIN2** | 26 | Direção B (Motor Direito) |
| **PWMB** | 25 | Velocidade PWM Motor Direito (Canal LEDC 5) |
| **STBY** | 33 | Standby da Ponte H (HIGH = Habilitado) |
| **TRIG** | 19 | Disparo do Sensor Ultrassônico HC-SR04 |
| **ECHO** | 21 | Retorno do Eco (usar divisor de tensão resistivo 5V -> 3.3V) |
| **SERVO** | 2 | Sinal PWM do Micro Servo SG90 |
| **BATERIA (Opcional)**| 255 | Defina no `Config.h` caso utilize divisor resistivo em ADC (ex: 34, 35) |

> [!WARNING]
> **Divisor de Tensão no ECHO**: O pino ECHO do HC-SR04 opera em 5V. O ESP32 tolera no máximo 3.3V. Sempre use divisor resistivo (ex: 2.2kΩ e 3.3kΩ) para proteger o GPIO 21.

---

## 4. Interface Web (Cyberpunk / Sci-Fi Tactical HUD)

Acesse conectando à rede Wi-Fi **Megatron** (senha: `12345678`) e abrindo no navegador:
**`http://192.168.4.1/`**

### Recursos da Interface:
- **Estética Cyberpunk HUD**: Paleta escura com neon ciano (`#00f2fe`), verde esmeralda (`#10b981`), âmbar (`#f59e0b`) e vermelho de alerta (`#ef4444`), tipografia Orbitron e JetBrains Mono, com bordas chanfradas táticas.
- **Radar & SLAM em Canvas HTML5**:
  - Anéis concêntricos de radar tático graduados em metros.
  - Varredura animada contínua de feixe LiDAR (FOV de 120º).
  - Células livres renderizadas em verde e obstáculos em vermelho vivo.
  - Linha de trajetória com brilho âmbar conectando os pontos percorridos.
  - **Click-to-Nav**: Clique em qualquer ponto da grade para criar um retículo pulsante e enviar o robô autonomamente para as coordenadas do Waypoint (`goto`).
- **Painéis de Telemetria**:
  - Posição atual `(X, Y)` em metros, orientação em graus e rosa dos ventos (N, L, S, O).
  - Células mapeadas e estimativa de área explorada em m².
  - Indicador de bateria com voltagem estimada e medidor de sinal/latência (ping WebSocket em ms).
  - Proximidade nos 4 quadrantes (Frente, Ré, Esquerda, Direita).
  - Horizonte Artificial (IMU) com leitura e inclinação de Roll, Pitch e Yaw.
- **Controles e Modos**:
  - D-Pad touch para celulares e tablets.
  - Atalhos de teclado no PC: `W` (Frente), `S` (Ré), `A` (Girar Esquerda), `D` (Girar Direita), `Espaço` (Parar).
  - Slider de velocidade PWM (0 a 255) com atalhos 35% (ECO), 65% (CRUISE) e 100% (TURBO).
  - Modos operacionais: **AUTO SLAM**, **MANUAL**, **RTH (Retorno à Base)** e **E-STOP (Parada de Emergência)**.
  - Terminal de logs com registros em tempo real de mensagens do sistema, pacotes TX/RX e alertas.

---

## 5. Protocolo de Comunicação WebSocket (`/ws`)

A comunicação opera em duplex de baixíssima latência através de WebSockets.

### Payload de Telemetria (ESP32 -> Web) [10 Hz]:
```json
{
  "x": 0.0,
  "y": 0.0,
  "dir": 0,
  "status": "AUTO SLAM",
  "action": "Avançando em linha reta",
  "battery": 88,
  "voltage": 7.8,
  "dist_fwd": 42.5,
  "dist_back": 120.0,
  "dist_left": 85.0,
  "dist_right": 70.0,
  "explored": 34,
  "pitch": -1.5,
  "roll": 0.0,
  "yaw": 0.0,
  "path": [[0,0], [0,1], [1,1]],
  "cells": [[0,0,1], [0,1,1], [0,2,2]]
}
```

### Comandos da Web (Web -> ESP32):
- **Movimento Manual**: `{"cmd": "move", "dir": "F"|"B"|"L"|"R"|"STOP", "speed": 0-255}`
- **Troca de Modo**: `{"cmd": "mode", "value": "manual"|"auto"|"rth"}`
- **Parada de Emergência**: `{"cmd": "estop"}`
- **Navegação para Waypoint**: `{"cmd": "goto", "x": 2.0, "y": 3.0}`
- **Reset de Mapa**: `{"cmd": "reset"}`
- **Ping / Latência**: `{"cmd": "ping", "t": 1730890000000}`
