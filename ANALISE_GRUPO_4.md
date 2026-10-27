# Análise do projeto — Grupo 4

Esta análise compara o código modularizado com os requisitos do trabalho de Robótica Móvel para o Grupo 4. Ela foi preparada a partir do PDF `Trabalho_PjBL_-_AV2 (1).pdf`, dos arquivos do sketch e do guia `LEIA_PRIMEIRO.md`.

## Resumo

O código controla os motores, mede distâncias com o sensor ultrassônico, movimenta o servo para observar os lados e escolhe uma direção usando regras. Agora também mantém um mapa em grade do ambiente e registra o percurso, o que atende ao requisito central do Grupo 4: exploração com memória e mapeamento simplificado.

A pose é estimada por tempo (não há encoders), então o mapa é uma aproximação que precisa de calibração física.

## O que já foi implementado

1. **Mapa em grade (`GridMap.h/.cpp`):** grade 21×21 em que cada célula pode estar desconhecida, livre ou com obstáculo. O robô começa no centro.
2. **Memória do percurso:** a célula inicial e as células percorridas são marcadas como livres; obstáculos à frente, à esquerda e à direita são marcados na grade.
3. **Pose estimada por tempo:** a cada `TEMPO_POR_CELULA_MS` de avanço o robô avança uma célula; cada giro muda o sentido em 90°. Como não há encoders, a posição é aproximada.
4. **Exploração com redução de repetição:** entre dois lados livres, a navegação escolhe a direção com mais células desconhecidas (`unknownAhead`), caindo para a comparação de espaço e para a preferência alternada em empate.
5. **Visualização do mapa:** o mapa é impresso no Monitor Serial no início, a cada `IMPRIMIR_MAPA_A_CADA_N_CELULAS` células e em cada decisão (giro ou beco sem saída).
6. **Correção do giro duplicado:** `reverse()` passou a informar quando já girou para sair do beco, evitando um segundo giro no cenário de frente e lados bloqueados.
7. **Tratamento de sem eco configurável:** `SEM_ECO_LIVRE` controla se ausência de eco conta como caminho livre (padrão `true`).
8. **Sketch renomeado:** `robo_re_varredura.ino` → `megatron.ino`, deixando o nome do arquivo igual ao da pasta, como exige a Arduino IDE.

## O que o código já atende

- **ESP32, motores e driver:** há controle dos dois motores e do pino de standby do driver.
- **Percepção de obstáculos:** há leitura de distância com o ultrassônico e varredura para frente, diagonais e lados.
- **Escolha automática de direção:** diante de um obstáculo, o código compara os lados e decide para onde girar.
- **Comportamento baseado em regras:** há regras para caminho bloqueado, obstáculo muito próximo, recuo e escolha entre os lados.
- **Organização modular:** a inicialização, configuração, controle dos motores, leitura do sensor, controle do servo e navegação foram separados em arquivos próprios. A arquitetura está mais clara e cada módulo tem uma responsabilidade principal.

Isso atende a integração, a navegação, o requisito de organização modular e a exploração com mapeamento descrita na seção anterior.

## O que falta para atender ao Grupo 4

| Prioridade | Requisito | Situação nos arquivos disponíveis |
|---|---|---|
| **Essencial — atendido** | Registrar regiões ou pontos percorridos | Implementado no `GridMap`: células livres/obstáculo e pose estimada por tempo. |
| **Essencial — atendido** | Representar o ambiente ou o percurso | Implementado: grade 21×21 impressa no Monitor Serial. |
| **Essencial — atendido** | Reduzir repetição e retorno a áreas exploradas | Implementado: a navegação prefere a direção com mais células desconhecidas. |
| **Essencial — atendido** | Exploração autônoma | Implementado: o histórico de células orienta a exploração; falta validar fisicamente. |
| **Importante** | Testes práticos e análise dos resultados | O guia descreve como testar, mas ainda não há resultados registrados, cenários executados ou análise de desempenho. |
| **Importante** | Protótipo funcional com estrutura modelada e impressa em 3D | Não há modelo CAD, fotos ou outra evidência disso nos arquivos disponíveis. O código, sozinho, não confirma esse requisito. |
| **Atendido** | Organização modular do software | O sketch foi separado em módulos (configuração, motores, sensor, servo, navegação e mapa) e compilou para ESP32 Dev Module. A validação física continua pendente. |
| **Entrega** | Relatório acadêmico e apresentação/demonstração | O guia técnico não substitui o relatório exigido pelo PDF, nem comprova a apresentação ou a demonstração final. |

## Requisitos gerais do trabalho escrito

Além dos requisitos específicos do Grupo 4, o PDF pede:

- contextualização do tema, finalidade e relevância;
- relação com Robótica Móvel, Engenharia de Software e Inteligência Artificial;
- fundamentação teórica sobre sensores, atuadores, controle, navegação, software embarcado, arquitetura e tomada de decisão;
- descrição técnica do protótipo, componentes, estrutura 3D, arquitetura, módulos de software, lógica e algoritmos;
- testes, cenários, resultados, limitações, problemas encontrados e análise crítica;
- conclusão;
- citações, referências e organização conforme as normas ABNT;
- entrega do trabalho escrito em PDF;
- apresentação oral e demonstração prática do protótipo funcional.

Esses itens não estão documentados como relatório nos arquivos disponibilizados.

## Divergências e pontos de atenção no código

1. **Leitura sem eco:** a ausência de eco é tratada como caminho livre, agora controlada pela constante `SEM_ECO_LIVRE` (padrão `true`). Ainda deve ser revalidada, pois uma leitura desconhecida não confirma que a passagem está segura.
2. **Filtro e distância-limite:** o código e o guia agora descrevem três amostras e um limite de 25 cm. Esse comportamento deve ser validado fisicamente e ajustado com base nos testes.
3. **Limite de giros:** não há contador de giros consecutivos; o limite atual é de duas tentativas de ré seguidas.
4. **Giro duplicado corrigido:** `reverse()` agora informa quando já girou para sair do beco, e `analyzeObstacle()` evita solicitar um segundo giro.
5. **Estimativa do percurso:** o mapeamento usa pose estimada por tempo (`TEMPO_POR_CELULA_MS` por célula), sem encoders. O mapa é aproximado e pode divergir do ambiente real; os tempos precisam de calibração.
6. **Teste físico:** a compilação da versão modular foi validada para ESP32 Dev Module com Arduino-ESP32 3.3.11 e ESP32Servo 3.2.1. Ainda é necessário testar o protótipo real e calibrar os tempos.

## Próximos passos recomendados

1. Calibrar `TEMPO_POR_CELULA_MS`, `TEMPO_GIRO_MS` e `TEMPO_RE_MS` com a velocidade real do robô para o mapa corresponder ao deslocamento físico.
2. Validar fisicamente o comportamento e registrar testes com cenários, resultados, limitações e melhorias.
3. Reavaliar `SEM_ECO_LIVRE` e os parâmetros do filtro e da distância-limite durante os testes.
4. Preparar o modelo 3D, o relatório acadêmico e a demonstração exigidos no PDF.

## Conclusão

O projeto agora integra sensores, motores, desvio de obstáculos e um mapa em grade que registra o percurso e orienta a exploração, atendendo ao requisito distintivo do Grupo 4 (mapeamento simplificado e redução da repetição de trajetórias). Restam a validação física do protótipo, a calibração dos tempos e as entregas escritas (relatório, modelo 3D e apresentação).
