# Megatron

Robô móvel de exploração autônoma com mapeamento simplificado do ambiente (Grupo 4, Robótica Móvel, AV2 PjBL).

- **Placa:** ESP32 WROOM (`ESP32 Dev Module`)
- **Motores:** dois motores DC com driver TB6612FNG
- **Percepção:** sensor ultrassônico HC-SR04 sobre um servo SG90 que faz a varredura
- **Comportamento:** avança, desvia de obstáculos olhando em cinco direções e registra o percurso em um mapa em grade 21×21, impresso no Monitor Serial (115200 baud)

## Como compilar

1. Abra `megatron.ino` na Arduino IDE (todos os `.h` e `.cpp` na mesma pasta).
2. Instale a biblioteca **ESP32Servo**.
3. Selecione a placa `ESP32 Dev Module` e envie.

Pinagem, ligação, comportamento, roteiro de testes e limitações estão em [LEIA_PRIMEIRO.md](LEIA_PRIMEIRO.md). Arquitetura, algoritmos do mapa e da escolha de direção, métricas e roteiro de testes de exploração estão em [DOCUMENTACAO_TECNICA.md](DOCUMENTACAO_TECNICA.md). A comparação com os requisitos do trabalho está em [ANALISE_GRUPO_4.md](ANALISE_GRUPO_4.md). Os parâmetros ajustáveis (pinos, ângulos, limites e tempos) ficam em `Config.h`.
