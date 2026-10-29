# Documentação técnica: robô explorador Megatron

Este documento descreve a arquitetura, os algoritmos e o roteiro de testes do robô. Ele serve de base para as Partes 3 (Projeto e Implementação) e 4 (Testes e Análise Crítica) do relatório. Montagem, pinagem e calibração estão em [LEIA_PRIMEIRO.md](LEIA_PRIMEIRO.md).

## 1. Requisitos do Grupo 4 e onde são atendidos

| Requisito | Onde está |
|---|---|
| Registro simplificado das regiões ou pontos percorridos | `GridMap`: cada célula guarda o estado (desconhecida, livre, obstáculo) e o número de passagens do robô (seção 4). |
| Mecanismo para reduzir repetição de trajetórias | `Navigation::compareExploration()`, usado na escolha de lado (seção 5). |
| Análise do comportamento durante a exploração | Linhas `STATS` em CSV no Monitor Serial e roteiro de testes (seções 6 e 7). |
| Documentação técnica e demonstração funcional | Este documento e o `LEIA_PRIMEIRO.md`. A demonstração é feita com o protótipo. |
| Representação simplificada do ambiente ou do percurso | Mapa em grade impresso no Monitor Serial (seção 4). |

## 2. Arquitetura

O software é dividido em módulos com uma responsabilidade cada. A navegação é a única que toma decisões; os demais módulos só executam ou medem.

```mermaid
flowchart TD
    INO[megatron.ino<br/>setup e loop] --> NAV[Navigation<br/>regras de decisão]
    NAV --> MOT[MotorController<br/>TB6612FNG]
    NAV --> SEN[DistanceSensor<br/>HC-SR04 e filtro]
    NAV --> SRV[ServoScanner<br/>varredura do sensor]
    NAV --> MAP[GridMap<br/>mapa, pose e visitas]
    SRV --> SEN
    CFG[Config.h<br/>pinos, limites e tempos] -.-> NAV
    CFG -.-> MOT
    CFG -.-> SEN
    CFG -.-> SRV
```

## 3. Ciclo de navegação

A cada volta do `loop()`, `Navigation::update()` faz uma leitura rápida da frente e decide:

```mermaid
flowchart TD
    A[Ler distância da frente] --> B{Obstáculo a até 25 cm?}
    B -- sim --> C[Parar e confirmar com leitura filtrada]
    C --> D{Confirmado?}
    D -- não --> A
    D -- sim, a até 10 cm --> R[Dar ré]
    R --> E
    D -- sim --> E[Olhar diagonais e lados]
    E --> F{Algum lado livre?}
    F -- um --> G[Girar para ele]
    F -- os dois --> H[Girar para o menos explorado]
    F -- nenhum --> I[Ré e giro: beco sem saída]
    B -- não --> J[Avançar e contar células]
    J --> A
```

## 4. Mapa em grade e registro do percurso

- **Grade:** 21 × 21 células. O robô começa no centro, voltado para o norte.
- **Estado de cada célula:** desconhecida, livre ou obstáculo. Obstáculos vistos à frente, à esquerda e à direita são gravados na célula vizinha correspondente.
- **Passagens:** cada célula livre guarda quantas vezes o robô entrou nela. Esse é o registro do percurso: células com número alto são regiões por onde ele repetiu caminho.
- **Pose estimada por tempo:** a cada `TEMPO_POR_CELULA_MS` (500 ms) de avanço, o robô anda uma célula; ao parar, conta a célula em andamento se já percorreu metade dela. Cada giro muda o sentido em 90°. Sem encoders, a pose é aproximada. Para reduzir o erro acumulado, uma célula em que o robô entra passa a ser livre, mesmo que antes estivesse marcada como obstáculo.

**Legenda do mapa impresso no Monitor Serial:**

| Símbolo | Significado |
|---|---|
| `.` | célula desconhecida |
| `#` | obstáculo |
| `1` a `9` | célula livre e número de passagens (9 = nove ou mais) |
| `^` `>` `v` `<` | robô e o sentido em que está (norte, leste, sul, oeste) |

Abaixo do mapa sai um resumo com as contagens, a posição relativa ao início e a direção.

## 5. Mecanismo de anti-repetição

Diante de um obstáculo, com os dois lados livres, o robô usa o mapa para escolher para onde ir (`compareExploration()`):

1. o lado com mais células desconhecidas nas próximas 3 células (`ALCANCE_AVALIACAO`);
2. em empate, o lado com menos passagens nessas células;
3. em novo empate, o lado com mais espaço medido;
4. por último, alterna o lado preferido.

Assim, sempre que há escolha, o robô tende a ir para onde ainda não esteve, em vez de voltar por um caminho já percorrido.

**Versão testada e removida:** uma versão anterior também fazia o robô parar durante o avanço, ao entrar numa célula já visitada duas vezes, para olhar os lados e procurar uma área menos explorada. No teste com o protótipo, num ambiente com obstáculos próximos, essa parada extra fazia o robô girar demais e travar, então foi retirada. É um resultado útil para a análise crítica: num espaço apertado, mais paradas para varrer custam mais tempo e manobras do que economizam em repetição.

## 6. Métricas para a análise (linhas STATS)

Ao ligar, o robô imprime o cabeçalho e, a cada impressão do mapa, uma linha CSV:

```
STATS,tempo_ms,visitadas,revisitas,obstaculos,giros,res
```

| Coluna | Significado |
|---|---|
| `tempo_ms` | tempo desde que a placa ligou |
| `visitadas` | células diferentes por onde o robô passou (área explorada) |
| `revisitas` | passagens além da primeira em cada célula (quanto ele repetiu caminho) |
| `obstaculos` | células marcadas como obstáculo |
| `giros` | giros de 90° realizados |
| `res` | manobras de ré |

**Como coletar:** ao fim de um teste, copie o conteúdo do Monitor Serial, mantenha só as linhas que começam com `STATS` e cole numa planilha, usando vírgula como separador. Gráficos úteis para o relatório:

- `visitadas` × `tempo_ms`: velocidade de exploração;
- `revisitas` × `tempo_ms`: quanto o robô repete caminho;
- razão `revisitas / visitadas` ao fim do teste: eficiência da exploração (quanto menor, melhor).

Para mostrar o efeito de uma mudança, repita o mesmo cenário com e sem ela (por exemplo, `VARRER_APOS_RE = true` e `false`), pelo mesmo tempo, e compare as métricas.

## 7. Roteiro de testes

Use sempre o mesmo ponto de partida, a mesma direção inicial e o mesmo tempo de teste (por exemplo, 3 minutos). Fotografe ou desenhe o ambiente real para comparar com o mapa impresso.

| # | Cenário | O que observar | Visitadas | Revisitas | Giros | Rés | Observações |
|---|---|---|---|---|---|---|---|
| 1 | Corredor reto com parede no fim | parada, escolha de lado, mapa coerente com o corredor | | | | | |
| 2 | Sala com um obstáculo no centro | contorno do obstáculo, cobertura da sala | | | | | |
| 3 | Mesma sala do cenário 2, com `VARRER_APOS_RE = false` | comparação com o cenário 2 | | | | | |
| 4 | Beco sem saída (caixa em U) | ré, giro e saída do beco | | | | | |
| 5 | Ambiente em U ou L | se o robô evita voltar pelo mesmo braço | | | | | |

Para cada teste, registrar também: se houve colisão, se o robô travou e quanto o mapa impresso diverge do ambiente real.

## 8. Limitações conhecidas

- **Pose por tempo:** derrapagem, nível da bateria e giros que não dão exatamente 90° acumulam erro no mapa. Calibre `TEMPO_POR_CELULA_MS` e `TEMPO_GIRO_MS` com o robô real.
- **Sensor único e direcional:** o HC-SR04 tem feixe largo e não detecta bem objetos pequenos, inclinados ou macios. Ausência de eco conta como livre (`SEM_ECO_LIVRE = true`).
- **Mapa local:** a decisão olha só 3 células adiante em cada direção. Não há planejamento de rota até a área desconhecida mais próxima (busca de fronteira), que seria a evolução natural.
- **Grade limitada:** 21 × 21 células; ao chegar à borda, a pose deixa de avançar no mapa.
- **Sem detecção traseira:** a ré é feita sem medir o que há atrás do robô.
