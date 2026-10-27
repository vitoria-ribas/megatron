# Robô explorador: varredura, desvio de obstáculos e mapeamento

Abra `megatron.ino` na Arduino IDE. Mantenha todos os arquivos `.h` e `.cpp` deste projeto na mesma pasta do sketch. Instale a biblioteca ESP32Servo (Kevin Harrington / John K. Bennett) e selecione uma placa ESP32 compatível. Esta versão é para ESP32 clássico/WROOM e servo SG90 de posição, não servo de rotação contínua.

## Organização do código

| Arquivo | Responsabilidade |
|---|---|
| `megatron.ino` | Inicialização do Serial e ciclo principal Arduino (`setup` e `loop`). |
| `Config.h` | Pinos, ângulos, limites e tempos ajustáveis. |
| `MotorController.h/.cpp` | Inicialização, parada e acionamento dos motores pelo TB6612FNG. |
| `DistanceSensor.h/.cpp` | Disparo do HC-SR04, conversão da distância e filtro de três leituras. |
| `ServoScanner.h/.cpp` | Inicialização do servo, posicionamento e leitura dos setores observados. |
| `Navigation.h/.cpp` | Regras de avanço, análise de obstáculos, ré, giro e exploração. |
| `GridMap.h/.cpp` | Grade de células, pose estimada e impressão do mapa no Serial. |

Os módulos se comunicam por interfaces de classe: a navegação usa os módulos de motores, sensor e servo, e registra o percurso no `GridMap`. A separação organiza o código e implementa memória de áreas visitadas com mapeamento simplificado do ambiente.

## Pinagem

| Sinal | GPIO |
|---|---:|
| AIN1 | 14 |
| AIN2 | 12 |
| PWMA | 13 |
| BIN1 | 27 |
| BIN2 | 26 |
| PWMB | 25 |
| STBY | 33 |
| TRIG | 19 |
| ECHO, através do divisor | 21 |
| Sinal do servo | 2 |

**GPIO12 (AIN2) e GPIO2 (servo) são pinos de boot do ESP32.** Se o GPIO12 estiver em nível alto ao ligar, a placa pode não iniciar; o GPIO2 também é lido no boot e costuma ter o LED da placa. Com a ligação atual isso não deve acontecer, mas, se o ESP32 às vezes não iniciar ou não aceitar upload com tudo conectado, desconecte esses dois fios, ligue a placa e reconecte. Se o problema se repetir, mova esses sinais para GPIOs livres e atualize `Config.h`.

Alimentação: ESP32 pelo USB; TB6612FNG VCC em 3V3, VM na alimentação compatível com os motores; HC-SR04 e servo em fonte de 5 V adequada aos modelos. Todos os GNDs devem estar interligados. Não alimente simultaneamente a placa pelo USB e pelo pino 5V/VIN. Evite conduzir a corrente dos motores pela protoboard.

**Nunca ligue ECHO diretamente ao ESP32.** Divisor recomendado: ECHO -> 2,2 kohms -> ponto de sinal; ponto de sinal -> GPIO21; ponto de sinal -> 3,3 kohms -> GND. Se forem usados três resistores de 1 kohm: um entre ECHO e sinal e dois em série entre sinal e GND (saída nominal de 3,33 V para entrada de 5 V).

## Comportamento implementado

- O sensor observa cinco posições: frente, duas diagonais e dois lados parciais. Os ângulos definidos em `Config.h` são comandos do servo e precisam ser conferidos fisicamente.
- As leituras filtradas usam três amostras. Com três distâncias válidas, é usada a mediana; com duas, a menor; com zero ou uma leitura válida, o resultado é tratado como sem eco.
- O robô avança enquanto a leitura frontal não confirma um obstáculo a até 25 cm. A medição frontal ocorre repetidamente durante o avanço; não há etapa de avanço temporizada de 250 ms.
- Um setor é considerado livre se a leitura for maior que 25 cm **ou se não houver eco**. Essa escolha atual pode tratar uma leitura desconhecida como caminho livre.
- Diante de obstáculo, o robô para, mede os setores laterais e escolhe uma direção quando a diagonal e o lado correspondente são considerados livres.
- O robô recua se a frente estiver a até 10 cm. Se frente e os dois lados estiverem bloqueados, executa a regra de ré e giro.
- A contagem de rés é limitada a duas seguidas e é zerada após pelo menos 500 ms de avanço. Não existe no código um contador de quatro giros nem bloqueio permanente até reset.
- Os tempos atuais de ré e giro são 350 ms (`TEMPO_RE_MS` e `TEMPO_GIRO_MS`), valores iniciais que precisam de calibração física.
- O robô mantém um mapa em grade (`GridMap`): cada célula pode estar desconhecida, livre ou com obstáculo. A célula inicial é marcada como livre.
- A pose é estimada por tempo: a cada `TEMPO_POR_CELULA_MS` de avanço, o robô avança uma célula no mapa; ao parar, a célula em andamento é contada se ele já percorreu pelo menos metade dela. Cada giro altera o sentido em 90°. Não há encoders, então a posição é aproximada.
- Uma célula por onde o robô passa é sempre marcada como livre, mesmo que antes tenha sido marcada como obstáculo, para corrigir parte da deriva da pose.
- Obstáculos detectados à frente, à esquerda ou à direita são marcados nas células correspondentes do mapa. As leituras diagonais orientam a navegação, mas não são gravadas na grade.
- Ao escolher entre dois lados livres, o robô prefere a direção com mais células desconhecidas à frente; em empate, compara o espaço disponível e, por fim, alterna a preferência (`preferLeft`).
- O mapa é impresso no Monitor Serial no início, a cada `IMPRIMIR_MAPA_A_CADA_N_CELULAS` células percorridas e sempre que o robô toma uma decisão (giro ou beco sem saída). A distância frontal durante o avanço é impressa a cada `INTERVALO_LOG_MS` (500 ms) ou sempre que há obstáculo.
- `SEM_ECO_LIVRE` controla se ausência de eco conta como caminho livre. O valor padrão é `true`; leia as limitações abaixo antes de mantê-lo.

## Como testar

1. Primeiro suspenda as rodas. O sensor deve olhar para a frente ao iniciar. Não force o eixo do servo.
2. Confira fisicamente esquerda e direita. Se o servo olhar para os lados trocados, altere `INVERTER_OLHAR` em `Config.h`. Se o giro estiver invertido, confira `MOTOR_A_NA_ESQUERDA`.
3. Mantenha `INVERTER_A` e `INVERTER_B` com os valores que fizeram as rodas avançarem.
4. Use caixas grandes para confirmar as leituras dos setores no Monitor Serial (115200 baud).
5. Teste frente bloqueada com um lado livre, frente e lados bloqueados e obstáculo muito próximo.
6. Teste no chão com a traseira livre e acesso ao desligamento. Não há sensor traseiro, sensor lateral contínuo ou detector de bordas/escadas; a leitura frontal não verifica o caminho durante a ré.

## Limitações e próximos passos

- Sem eco é tratado como livre pela lógica atual (`SEM_ECO_LIVRE = true`). É necessário rever essa política para não assumir que uma leitura desconhecida confirma passagem segura.
- O giro duplicado do beco sem saída foi corrigido: `reverse()` agora informa quando já girou para sair do beco, e `analyzeObstacle()` evita solicitar um segundo giro.
- Os movimentos são temporizados e não usam encoders. O programa não mede deslocamento real, derrapagem nem posição; o mapa em grade é uma estimativa que pode divergir do ambiente real.
- A ré não atualiza a pose no mapa, pois é curta e serve para reposicionar o robô dentro da mesma célula.
- O HC-SR04 tem feixe com largura angular; leituras em cinco posições não reconstroem o contorno nem garantem a largura de uma passagem. Objetos pequenos ou inclinados podem não ser detectados.
- As medidas de 10 cm por 4 cm foram informadas, mas não está confirmado se são dimensões do robô ou do objeto. O código não as usa para validar passagens. Meça o robô completo, incluindo rodas, suporte e eixo de giro, antes de ajustar as margens.
- O mapeamento por tempo depende de `TEMPO_POR_CELULA_MS` (padrão 500 ms); calibre esse valor com a velocidade real do robô para que uma célula corresponda ao deslocamento físico desejado.
- O sketch foi compilado para `ESP32 Dev Module` com Arduino-ESP32 3.3.11 e ESP32Servo 3.2.1. A compilação apresentou avisos dentro da biblioteca ESP32Servo, mas terminou com sucesso. A validação física do protótipo continua necessária.
