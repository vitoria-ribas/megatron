# Análise do projeto — Grupo 4

Esta análise compara o código modularizado com os requisitos do trabalho de Robótica Móvel para o Grupo 4. Ela foi preparada a partir do PDF `Trabalho_PjBL_-_AV2 (1).pdf`, dos arquivos do sketch e do guia `LEIA_PRIMEIRO.md`.

## Resumo

O código já é uma boa base de desvio de obstáculos: controla os motores, mede distâncias com o sensor ultrassônico, movimenta o servo para observar os lados e escolhe uma direção usando regras. Porém, o requisito central do Grupo 4 ainda não está completo: falta implementar a exploração com memória e mapeamento simplificado do ambiente.

No estado atual, o projeto se aproxima mais de um robô que desvia de obstáculos do que de um robô que explora e mapeia o ambiente.

## O que o código já atende

- **ESP32, motores e driver:** há controle dos dois motores e do pino de standby do driver.
- **Percepção de obstáculos:** há leitura de distância com o ultrassônico e varredura para frente, diagonais e lados.
- **Escolha automática de direção:** diante de um obstáculo, o código compara os lados e decide para onde girar.
- **Comportamento baseado em regras:** há regras para caminho bloqueado, obstáculo muito próximo, recuo e escolha entre os lados.
- **Organização modular:** a inicialização, configuração, controle dos motores, leitura do sensor, controle do servo e navegação foram separados em arquivos próprios. A arquitetura está mais clara e cada módulo tem uma responsabilidade principal.

Isso atende parte da integração, da navegação e do requisito de organização modular, mas ainda não caracteriza a exploração com mapeamento pedida para o Grupo 4.

## O que falta para atender ao Grupo 4

| Prioridade | Requisito | Situação nos arquivos disponíveis |
|---|---|---|
| **Essencial** | Registrar regiões ou pontos percorridos | Não há memória de posições, células ou trechos já visitados. |
| **Essencial** | Representar o ambiente ou o percurso | Não há mapa, matriz ou representação do trajeto gerada pelo código. |
| **Essencial** | Reduzir repetição e retorno a áreas exploradas | Alternar a preferência entre esquerda e direita não identifica áreas visitadas nem evita repetir o mesmo caminho. |
| **Essencial** | Exploração autônoma | O robô reage aos obstáculos, mas não mantém um histórico para orientar a exploração. |
| **Importante** | Testes práticos e análise dos resultados | O guia descreve como testar, mas os arquivos disponíveis não apresentam resultados registrados, cenários executados ou análise de desempenho. |
| **Importante** | Protótipo funcional com estrutura modelada e impressa em 3D | Não há modelo CAD, fotos ou outra evidência disso nos arquivos disponíveis. O código, sozinho, não confirma esse requisito. |
| **Atendido** | Organização modular do software | O sketch foi separado em módulos de configuração, motores, sensor, servo e navegação, e compilou para ESP32 Dev Module. A validação física continua pendente. |
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

1. **Leitura sem eco:** a lógica interpreta ausência de eco como caminho livre e pode permitir que o robô avance. Isso deve ser revisto porque ausência de leitura não confirma que a passagem está segura.
2. **Filtro e distância-limite:** o código e o guia agora descrevem três amostras e um limite de 25 cm. Esse comportamento deve ser validado fisicamente e ajustado com base nos testes.
3. **Limite de giros:** não há contador de giros consecutivos; o limite atual é de duas tentativas de ré seguidas.
4. **Possível giro duplicado:** ao atingir o limite de rés no cenário em que a frente e os dois lados estão bloqueados, a função de ré já realiza um giro e a lógica do cenário solicita outro. Isso pode provocar dois giros consecutivos.
5. **Estimativa do percurso:** os movimentos são controlados por tempo, sem encoders. Assim, o código não sabe com precisão quanto avançou nem se voltou a uma região anterior. Essa limitação precisa ser considerada ao planejar o mapeamento.
6. **Teste físico:** a compilação da versão modular foi validada para ESP32 Dev Module com Arduino-ESP32 3.3.11 e ESP32Servo 3.2.1. Ainda é necessário testar o protótipo real e calibrar os tempos.

## Próximos passos recomendados

1. Implementar uma memória simples do percurso, por exemplo, uma grade de células ou uma lista de posições aproximadas.
2. Gerar e exibir uma representação do mapa ou do trajeto, por exemplo, no Monitor Serial.
3. Usar o registro das áreas visitadas para escolher direções menos exploradas e reduzir repetições.
4. Rever o tratamento de ausência de eco e validar os parâmetros do filtro, distância-limite e movimento.
5. Verificar o comportamento físico do protótipo e registrar testes com cenários, resultados, limitações e melhorias.
6. Preparar o relatório acadêmico e a demonstração exigidos no PDF.

## Conclusão

O projeto já tem uma base de sensores, motores e desvio de obstáculos. A principal lacuna é transformar essa reação local em exploração com registro e representação do ambiente. Sem isso, o requisito distintivo do Grupo 4 — mapeamento simplificado e redução da repetição de trajetórias — ainda não está atendido.
