# Buzzer passivo - GPIO 16 (RX2)

- Placa alvo: ESP32 classico (esp32dev), Arduino via PlatformIO original.
- PWM no canal LEDC 14, grupo low-speed, separado dos canais automaticos dos motores. Nenhuma alteracao de configuracao de PWM dos motores.
- A inicializacao e a atualizacao do som sao adicionadas em main.cpp. O sequenciador usa millis(), sem delay. NavigationCompleted() apenas le o estado DONE. Os pontos de escrita dos motores apenas informam o sentido aplicado ao buzzer; nenhum comando original foi removido.
- Bateria baixa: 30 s, critica: 5 s, com prioridade sobre outros sons; usa os estados existentes, sem alterar limites ou corte.
- Re: ambas as rodas comandadas para tras, nao giro; o aviso termina quando esse comando cessa. A demonstracao web de re tem 3 bipes.
- Vitoria: frequencias convertidas das notas da imagem fornecida; duracoes aproximadas escolhidas porque a imagem nao fornece ritmo. Nao ha efeito de missao interrompida.
- Em http://192.168.4.1/ abra a aba Sons / Buzzer. Os botoes apenas solicitam audio. Testes sonoros somente com robo parado e sem captura. O botao de parar cancela apenas testes, nao alertas reais.
- Rota POST /api/buzzer/test, parametro efeito: boot, low, critical, start, reverse, victory. POST /api/buzzer/stop cancela o teste.
- Antes de conectar/energizar, confirme se o buzzer e piezoeletrico passivo de baixa corrente e compativel com 3,3 V; resistor 100 ohms nao garante sozinho corrente segura. Se eletromagnetico ou de corrente superior a permitida pelo GPIO, use acionamento adequado conforme datasheet e supervisao de bancada.

## Validacao realizada

- Teste host do novo sequenciador: `g++ -std=c++17 -Wall -Wextra -Werror -Isrc/firmware tests/host/test_buzzer.cpp -o /tmp/test_buzzer && /tmp/test_buzzer`.
- JavaScript da pagina: verificacao sintatica por Node.
- Suite host original: maze, maze_planner_v48, nav_laser_policy e nav_opening passam; navigation_v48 falha na mesma assercao antes/depois (espera distancia exatamente NAV_CELULA_MM, enquanto a geometria atual permite distancia diferente). Nao corrigido para preservar a navegacao.
- Compilacao ESP32/PlatformIO e teste fisico nao realizados neste ambiente (toolchain indisponivel). Verificar antes de gravar e testar com robo parado, rodas livres e chave acessivel.
