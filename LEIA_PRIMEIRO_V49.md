# Xaropinho V4.9 — navegação goal / reverse / corner

Versão candidata construída a partir de `Micromouse_V48.zip` recebido em 17/11/2026. Não é uma versão homologada para o hardware. A eletrônica, IP da telemetria (192.168.4.6), modo de aquisição dos VL53L0X, offsets, CPRs/NVS, ganhos PI e rotinas de giro foram preservados. O adaptador de movimento e o controlador reto receberam extensões limitadas para as novas funções. As variáveis `NAV_SEM_ALVO_*`, ausentes no config enviado, foram declaradas.

## Atualizar sem reinstalar o backend

Preferir o pacote de atualização: extrair em uma pasta fora do projeto e executar (substitua o caminho da pasta descompactada se necessário):

```bash
python3 ~/Downloads/Xaropinho_V49_ATUALIZACAO/apply_v49.py ~/PI1/Micromouse_V48
python3 ~/Downloads/Xaropinho_V49_ATUALIZACAO/apply_v49.py ~/PI1/Micromouse_V48 --apply
```

O primeiro comando apenas verifica os hashes dos arquivos. O segundo cria cópia de segurança e atualiza os arquivos. Se algum arquivo mudou depois do ZIP, o instalador interrompe antes de copiar para evitar perder alterações. Backend `.env`, dependências e dados não são modificados. O projeto completo sem caches/credenciais do banco também está disponível como alternativa. Não sobreponha projetos completos em uma mesma pasta.

## Testes sem placa e compilação

```bash
cd ~/PI1/Micromouse_V48
bash tests/host/run_v49.sh
pio run -e esp32dev
```

Se a compilação ESP32 passar, preserve o meio de corte físico da alimentação dos motores. Grave a placa pelo USB com os motores desabilitados e a supervisão do responsável pelo laboratório. A ferramenta PlatformIO/ESP32 completa não estava disponível no ambiente em que o projeto foi preparado. Os testes executados são C++ no host com mocks. O JavaScript do painel passou em `node --check`. O frontend não foi compilado neste ambiente.

## Algoritmo

- Início fixo em (0,0), rumo N. Para 8 x 4, destino padrão (7,3); o painel recalcula o canto oposto quando as dimensões mudam. Outros objetivos permanecem disponíveis para testes curtos.
- BFS sobre passagens confirmadas. Prioridade: objetivo já acessível; senão célula ainda não visitada que minimize distância conhecida + Manhattan ao destino. Não garante um ótimo global antes de conhecer o mapa; encerra ao atingir o destino, não ao mapear tudo.
- Travessia é registrada separadamente de uma passagem apenas observada. Em retorno diretamente atrás do robô, só se permite ré em uma aresta realmente percorrida. Orientação permanece a mesma. Replanejamento em cada centro de célula. O retorno físico é experimental e não tem sensor traseiro. O perfil de ré começa desligado; somente o responsável pelo laboratório deve autorizar depois de validar o percurso estático e a parada física.
- Correção lateral opcional: usa centralizador já existente (duas paredes, somente esquerda, somente direita, ou encoder quando sem referência). A referência aprendida é guardada na RAM ao longo da missão. A correção vale no avanço; na ré, encoder e supervisão de yaw, sem malha lateral. A IMU verifica desvio e conclusão de giro; não é uma nova malha de direção.
- Estimativa de abertura: guarda posição do encoder na transição de duas leituras de parede para duas leituras válidas de abertura; interpola entre última parede e primeira abertura. Estimativa do centro = borda + largura livre/2 + deslocamento longitudinal do sensor projetado no sentido do movimento. Valores de U2/U4 positivos se sensor à frente do eixo, negativos se atrás. A posição de montagem e largura livre são parâmetros físicos, não podem ser inferidos inequivocamente de uma única transição. O cálculo é automático durante a marcha. Janela de ajuste de até ±12 mm, com margem de 25 mm antes do novo alvo e do nominal, apenas se o mapa já confirma abertura no lado e na célula de destino. Em ré não alonga o percurso. Caso contrário é somente diagnóstico ou segue a distância nominal. Não é localização absoluta.
- Uma leitura `SINAL_FRACO` não prova que o corredor está livre. Inicialização exige os sensores inicializados, não uma parede visível; observação parcial permite uma alternativa conhecida após esperar os sensores. Por padrão `NAV_SEM_ALVO_COMO_PASSAGEM=false`. Se o único caminho estiver indefinido, a missão para. Não desabilitar esse bloqueio para obter sucesso artificial. A transição de abertura também exige evidência válida. Timeouts, erros I2C durante avanço, perda de IMU, bateria e desvio do controlador continuam bloqueando movimentos; em falha no meio de manobra, a pose é invalidada. Parada de software não substitui corte físico dos motores.

## Painéis

Backend e frontend existentes podem permanecer funcionando. Depois de gravar, o painel da ESP32 `/missao` tem dimensões 8 x 4 e objetivo 7,3 por padrão, e perfis opcionais por missão sem regravação. O site externo `/labirinto` mostra mapa, estado e contagem de recuos. As opções novas começam desligadas no hardware. Preencha as dimensões físicas apenas com valores medidos. Para observar uma estimativa sem alterar o alvo, informe a geometria e deixe o ajuste desligado. A confirmação do responsável é necessária para opções experimentais. Ela não substitui os ensaios físicos.

A API GET `/api/nav/status` inclui `poseConfiavel`, `passosRe`, `reHabilitada`, `centralizacaoHabilitada`, `ajusteAberturaHabilitado`, `ajusteAplicado`, `bordaEsqMm`, `bordaDirMm`, `centroEstimadoMm`, `metaDeslocamentoMm`, `referenciaLateralMm`. `null` significa dado indisponível. O mapa mantém o formato anterior para compatibilidade. O eixo deve estar no centro de (0,0) e o robô voltado N antes de cada nova missão; o mapa é reiniciado. Não retomar automaticamente a partir de pose incerta.

## Validação física mínima (com responsável)

1. Sem motores, confirmar painel, leitura dos sensores e tensão; registrar valores para parede e abertura, testar parede real em todas as posições. Confirmar geometria, folga para todo o giro e funcionamento do desligamento físico.
2. Responsável verifica individualmente parada, deslocamento e giro dos controladores existentes, sem iniciar missão ampla. Atrasos do sensor e sobrecurso real devem caber na margem. A medida enviada U3→nariz de 60 mm e passo de 180 mm foram preservados, não verificados. O limiar dianteiro é aproximadamente 91 mm na configuração recebida; se a parede estiver mais próxima antes de atingir o centro, o robô pode interromper. Não reduzir a proteção para mascarar esse problema.
3. Responsável valida trecho curto e retorno em corredor estático já atravessado, sem obstáculos atrás, antes de permitir o perfil de ré. Verificar referência lateral e estimativa da abertura em diagnóstico antes de permitir correção. Se o sensor lateral não der leituras claras na abertura, não há estimativa confiável.
4. Só após condições anteriores aprovadas, avaliar percurso 8x4 do canto inicial ao oposto. Um novo percurso exige reposicionar o robô. Não há garantia de completar todo labirinto físico; faltam validação mecânica, calibração geométrica e eventual solução de sensoriamento para corredores sem retorno óptico confiável.

## Testes executados

Ver `RESULTADOS_V49.txt`. Nove executáveis C++ passaram, incluindo missão fictícia 8x4 com beco sem saída, 14 deslocamentos (2 em ré), 2 giros, ajuste simulado de abertura e objetivo atingido. É uma simulação com posição e sensores ideais, não uma medição do hardware. O ensaio de movimento testa também limites do retargeting; o de centralização inclui uma e nenhuma parede. Não implementados: otimização por tempo de giro/velocidade, persistência do mapa, recuperação automática de pose após falhas e sensor traseiro. A V4.9 não tem como garantir todas as situações possíveis.
