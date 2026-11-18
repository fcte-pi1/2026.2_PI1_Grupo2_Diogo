#pragma once
#include <Arduino.h>
#include "robot_geometry.h"

// ===== V5.0: GEOMETRIA MEDIDA DO ROBO (18/11/2026) =====
// As SEIS medidas abaixo sao a unica fonte de geometria do firmware. Tudo o
// que a navegacao precisa (ponto de giro, limite frontal, meta de cada reta)
// e DERIVADO delas em robot_geometry.h. Antes disso o codigo carregava
// estimativas soltas (laser lateral "a 53 mm do eixo", corredor "168 mm") que
// nunca foram medidas e erravam o ponto de giro em quase um centimetro.
// Ao trocar de chassi, remedir estas seis e nao mexer em mais nada.
constexpr float ROBO_LARGURA_MM = 110.0f;          // lateral a lateral
constexpr float ROBO_COMPRIMENTO_MM = 120.0f;      // traseira a frontal
constexpr float ROBO_LATERAL_AO_NARIZ_MM = 40.0f;  // recuo da janela do U2/U4
constexpr float ROBO_U3_AO_NARIZ_MM = 60.0f;       // recuo da janela do U3
// Leitura REPORTADA pelo U3 com o robo tocando a parede. Nao e desajuste do
// sensor: e o recuo fisico da janela dentro do chassi. O firmware nunca pode
// deixar a leitura frontal cair abaixo deste valor.
constexpr float ROBO_U3_TOCA_MM = 60.0f;
// Ensaios no labirinto: a partir do instante em que o laser lateral perde a
// parede, quanto o robo ainda anda ate o eixo ficar no centro da abertura.
constexpr float ROBO_BORDA_GIRO_FRENTE_MM = 140.0f;
constexpr float ROBO_BORDA_GIRO_RE_MM = 20.0f;
inline GeometriaRobo geometriaRobo() {
    GeometriaRobo g;
    g.larguraMm = ROBO_LARGURA_MM;
    g.comprimentoMm = ROBO_COMPRIMENTO_MM;
    g.lateralAoNarizMm = ROBO_LATERAL_AO_NARIZ_MM;
    g.u3AoNarizMm = ROBO_U3_AO_NARIZ_MM;
    g.u3TocaMm = ROBO_U3_TOCA_MM;
    g.bordaAteGiroFrenteMm = ROBO_BORDA_GIRO_FRENTE_MM;
    g.bordaAteGiroReMm = ROBO_BORDA_GIRO_RE_MM;
    return g;
}
// Derivadas em tempo de compilacao (as mesmas contas de robot_geometry.h).
constexpr float ROBO_LATERAL_AO_EIXO_MM = 0.5f * (ROBO_BORDA_GIRO_FRENTE_MM - ROBO_BORDA_GIRO_RE_MM);
constexpr float ROBO_MEIA_ABERTURA_MM = 0.5f * (ROBO_BORDA_GIRO_FRENTE_MM + ROBO_BORDA_GIRO_RE_MM);
constexpr float ROBO_EIXO_AO_NARIZ_MM = ROBO_LATERAL_AO_NARIZ_MM + ROBO_LATERAL_AO_EIXO_MM;
constexpr float ROBO_EIXO_A_TRASEIRA_MM = ROBO_COMPRIMENTO_MM - ROBO_EIXO_AO_NARIZ_MM;
constexpr float ROBO_U3_AO_EIXO_MM = ROBO_EIXO_AO_NARIZ_MM - ROBO_U3_AO_NARIZ_MM;


// === NOVA ELETRONICA - REMAPEAMENTO DE 03/11/2026 ===
// CANAL A (AO1/AO2) = MOTOR ESQUERDO; encoder esquerdo C1=32 C2=33.
// CANAL B (BO1/BO2) = MOTOR DIREITO; encoder direito C1=14 C2=13.
// TB6612FNG: AO1/AO2 e BO1/BO2 sao SAIDAS da ponte H (nao pinos da ESP32).
// IMPORTANTE: GPIOs de CONTROLE da ponte H abaixo foram mantidos da V4.4,
// pois a nova mensagem so confirmou as SAIDAS de motor. CONFIRMAR na placa.
// GPIO 2, 4, 5 e 15 sao pinos de strap no ESP32 classico; confira o boot.
#define PIN_STBY 5
#define PIN_PWMA 15
#define PIN_AIN1 2
#define PIN_AIN2 4
#define ENC_ESQ_A 32
#define ENC_ESQ_B 33
#define PIN_PWMB 25
#define PIN_BIN1 26
#define PIN_BIN2 27
#define ENC_DIR_A 14
#define ENC_DIR_B 13

#define PIN_SDA 21
#define PIN_SCL 22

// Polaridade dos ENCODERS mantida da V4.3/V4.4 porque C1/C2 continuam
// nos mesmos GPIOs. CONFIRMAR contagens positivas para a frente antes de girar.
#define SINAL_ENCODER_ESQ 1
#define SINAL_ENCODER_DIR -1

// Polaridades PROVISORIAS do comando "frente" por roda fisica.
// Mantidas da convencao logica antiga ao trocar os canais A/B.
// Se a roda girar para tras ao comandar frente, INVERTA apenas o bool da roda.
constexpr bool ESQ_FRENTE_IN1_ALTO = false; // AIN1 LOW, AIN2 HIGH
constexpr bool DIR_FRENTE_IN1_ALTO = true;  // BIN1 HIGH, BIN2 LOW

// Valores geométricos NOMINAIS: NÃO são calibração concluída.
constexpr float DIAMETRO_RODA_MM = 34.0f;
constexpr float ENTRE_RODAS_MM = 100.0f;

constexpr unsigned int PWM_FREQUENCIA_HZ = 10000;
constexpr unsigned int PWM_RESOLUCAO_BITS = 8;
constexpr int PWM_LIMITE_TESTE = 180;  // teto provisório de segurança
constexpr uint32_t AMOSTRA_ENCODER_MS = 20;
constexpr uint32_t PERIODO_CONTROLE_MS = 20;
constexpr uint32_t AMOSTRA_BATERIA_MS = 100;  // mesmo ritmo do monitor de bateria anterior
constexpr uint32_t TESTE_DURACAO_MAX_MS = 10000;
constexpr uint32_t TESTE_DURACAO_MIN_MS = 250;
constexpr uint32_t WATCHDOG_WEB_MS = 1200;

// V3.1: limites de bancada; primeiro com rodas suspensas; solo somente em area livre.
constexpr float RPM_MAX_TESTE = 120.0f;
constexpr float RPM_MAX_POSICAO = 75.0f;
constexpr float RPM_ACELERACAO_POR_SEG = 160.0f;
constexpr float REV_DESACEL_POR_SEG2 = 2.4f;
constexpr int PWM_LIMITE_CONTROLE = 130;
// Na V3 o piso fixo de 35 impedia regular 40 RPM sem carga.
// V3.1 permite duty abaixo de 35, com ajuda TEMPORARIA de partida.
constexpr int PWM_MINIMO_CONTROLE = 0;
constexpr int PWM_AJUDA_PARTIDA = 65;
constexpr int PWM_AJUDA_PARTIDA_CURTA = 55;
constexpr int PWM_AJUDA_APROXIMACAO = 43;
constexpr uint32_t PARTIDA_SEM_PULSOS_MS = 140;
// V5.1: a ajuda acabava em 850 ms, mas a falha por travamento so dispara em
// CONTROLE_STALL_MS (1200 ms). Nos ultimos 350 ms a roda presa ficava SEM ajuda,
// so com o PI, que nesse tempo nao passava de ~66 de PWM (26% de duty). No ar
// isso basta; no chao, com os 285 g do robo, nao. Agora a ajuda cobre a janela
// inteira e SOBE em rampa (ver ajudaPartidaEscalonada em partida_logic.h).
constexpr uint32_t PARTIDA_AJUDA_ATE_MS = 1150;
// Ate aqui a ajuda sobe do valor de partida ate PWM_AJUDA_MAXIMA; depois segura.
constexpr uint32_t PARTIDA_AJUDA_RAMPA_ATE_MS = 700;
// Teto da ajuda escalonada. Fica abaixo de PWM_LIMITE_CONTROLE (130). So e
// alcancado se a roda continuar presa; ao primeiro pulso a ajuda e cancelada.
constexpr int PWM_AJUDA_MAXIMA = 115;
// V5.2: nos giros curtos pela IMU (correcao de 1 a 30 graus e alinhamento) a
// ajuda cheia dava um tranco que passava do alvo e esgotava as correcoes.
constexpr int PWM_AJUDA_MAXIMA_CORRECAO = 80;
// Partida conjunta (#152). O piso precisa ficar acima dos 12 RPM que a ajuda exige.
constexpr int64_t PARTIDA_PULSOS_MIN = 3;
constexpr float PARTIDA_RPM_PISO = 18.0f;
constexpr float PARTIDA_ADIANTAMENTO_MAX_MM = 1.0f;
constexpr float PARTIDA_JANELA_MM = 50.0f;
constexpr float KP_RPM = 0.40f;                 // parametros experimentais
constexpr float KI_RPM = 1.25f;                 // PWM/(RPM*s)
constexpr float FF_PWM_BASE = 8.0f;
constexpr float FF_PWM_POR_RPM = 0.44f;
constexpr float LIMITE_INTEGRAL_PWM = 70.0f;
constexpr int POSICAO_TOLERANCIA_CONTAGENS = 4;
constexpr int POSICAO_META_MAX_CONTAGENS = 2000;
constexpr uint32_t CONTROLE_STALL_MS = 1200;
static_assert(PARTIDA_AJUDA_ATE_MS < CONTROLE_STALL_MS,
              "a ajuda de partida precisa agir antes de a falha por travamento disparar");
static_assert(PARTIDA_AJUDA_RAMPA_ATE_MS < PARTIDA_AJUDA_ATE_MS, "rampa da ajuda maior que a janela");
constexpr uint32_t FREIO_MEDICAO_FINAL_MS = 1000;
// Freio ativo (#163): desligado ate comparar com o coast na bancada.
constexpr bool FREIO_ATIVO_PADRAO = false;
constexpr uint32_t FREIO_ATIVO_PARADO_MS = 100;  // solta depois de 100 ms sem pulsos
constexpr uint32_t FREIO_ATIVO_MAX_MS = 600;     // abaixo dos 1000 ms para um novo inicio

constexpr char WIFI_SSID[] = "Xaropinho-WiFi";
constexpr char WIFI_PASSWORD[] = "pi12pi34";

// Por segurança, o OTA ficará DESLIGADO enquanto esta senha não for alterada.
// Coloque uma senha forte e diferente da senha Wi-Fi para habilitar o OTA.
constexpr char OTA_PASSWORD[] = "DEFINA_SENHA_OTA_AQUI";

// ===== V4 GIROS COMPARATIVOS =====
// MPU-6050 com face superior para cima e eixo Z vertical.
// Se o Yaw manual indicar sinal inverso, ajuste para +1 APOS validar fisicamente.
constexpr int IMU_SINAL_DIREITA = -1;
constexpr uint32_t IMU_INTERVALO_US = 5000;  // ~200 Hz
constexpr uint32_t IMU_MAX_IDADE_MS = 100;
constexpr int IMU_AMOSTRAS_BIAS = 800;     // ~4 s a 200 Hz, com o robo IMOVEL
constexpr int IMU_TENTATIVAS_CALIBRACAO = 3; // repete sozinha se o robo se mexer
constexpr float IMU_RUIDO_MAX_DPS = 1.4f;
constexpr float IMU_ZONA_MORTA_DPS = 0.25f; // taxa abaixo disso nao entra no yaw
constexpr float IMU_TAXA_PARADO_MAX_DPS = 4.0f;
constexpr float IMU_TOLERANCIA_ANGULO_GRAUS = 1.5f;
constexpr float IMU_ACEL_ANGULAR_DESACEL_DPS2 = 270.0f;
constexpr float GIRO_RPM_PADRAO = 40.0f;
constexpr float GIRO_RPM_MAX = 60.0f;
constexpr float GIRO_SYNC_K_RPM_POR_MM = 2.8f;
constexpr float GIRO_SYNC_CORRECAO_RPM_MAX = 10.0f;
constexpr float GIRO_SYNC_PAUSA_MM = 3.2f;   // trava a roda adiantada
constexpr float GIRO_SYNC_RETOMADA_MM = 1.6f;
constexpr float GIRO_SYNC_FALHA_MM = 13.0f;
constexpr uint32_t GIRO_SYNC_FALHA_MS = 600;
constexpr float GIRO_LIMITE_DESLOCAMENTO_REL = 1.28f; // IMU nao pode girar indefinidamente
// Giro de angulo qualquer (correcao depois do giro de 90 graus).
constexpr float GIRO_ANGULO_MIN_GRAUS = 0.5f;
constexpr float GIRO_ANGULO_MAX_GRAUS = 180.0f;
// Em angulos pequenos o limite relativo de curso ficaria menor que a folga do
// proprio controle; vale o maior entre o relativo e alvo + esta margem.
constexpr float GIRO_MARGEM_CURSO_MIN_MM = 6.0f;

// ===== Correcao do angulo pelo giroscopio depois do giro (issue #153) =====
// Depois do giro de 90 graus, espera o robo parar, mede o erro pelo yaw e,
// se passar da tolerancia, faz um giro curto pela IMU. Valores iniciais:
// ajustar na bancada com gabarito externo.
constexpr float GIRO_CORRECAO_TOLERANCIA_GRAUS = 1.0f; // erro aceito sem corrigir
// V5.2: 15 -> 30 graus e 2 -> 3 tentativas. No chao o giro por encoder erra
// mais que na bancada (escorregamento do apoio dianteiro); com 15 graus o robo
// desistia de corrigir justamente quando mais precisava. A trilha aprendida
// (abaixo) reduz esse erro a partir do segundo giro.
constexpr float GIRO_CORRECAO_ERRO_MAX_GRAUS = 30.0f;  // acima disso nao tenta corrigir
constexpr uint8_t GIRO_CORRECAO_MAX_TENTATIVAS = 3;
constexpr float GIRO_CORRECAO_RPM = 20.0f;
constexpr float GIRO_CORRECAO_PARADA_GRAUS = 0.4f;     // corta o PWM faltando isso para o alvo
constexpr float GIRO_CORRECAO_PARADO_DPS = 1.0f;       // robo parado: taxa abaixo disso...
constexpr uint32_t GIRO_CORRECAO_PARADO_MS = 100;      // ...por este tempo seguido
constexpr uint32_t GIRO_CORRECAO_ESPERA_MIN_MS = 300;  // espera minima depois de cortar o PWM
constexpr uint32_t GIRO_CORRECAO_ESPERA_MAX_MS = 1500; // se nao parar ate aqui, termina sem corrigir

// ===== V4.3 - GIRO COM SINCRONIZACAO SUAVE =====
// Correcao proporcional limitada; nao freia individualmente uma roda.
// Valores conservadores para testes com o centro do eixo marcado no chao.
constexpr float GIRO_SUAVE_K_RPM_POR_MM = 3.0f;
constexpr float GIRO_SUAVE_CORRECAO_FRAC_MAX = 0.30f;
constexpr float GIRO_SUAVE_CORRECAO_RPM_MAX = 8.0f;
constexpr float GIRO_SUAVE_TAXA_CORRECAO_RPM_S = 75.0f;
constexpr float GIRO_SUAVE_DESACEL_INICIO_MM = 18.0f;
constexpr float GIRO_SUAVE_RPM_PERTO_META = 25.0f;
// V5.2: 6 mm/250 ms -> 12 mm/400 ms. Valores de bancada (eixo marcado no chao).
// No labirinto uma roda patina mais que a outra a cada partida. 12 mm de
// diferenca desloca o centro do robo ~6 mm, contra 25 mm de folga lateral; o
// ANGULO continua conferido pela IMU no fim. Abaixo disso o giro falhava por
// uma diferenca que nao afeta a navegacao.
constexpr float GIRO_SUAVE_FALHA_MM = 12.0f;
constexpr uint32_t GIRO_SUAVE_FALHA_MS = 400;
// V5.2: dessincronia aceita no FIM do giro suave (antes usava GIRO_SYNC_PAUSA_MM,
// 3,2 mm, e caia em "giro_fim_com_dessincronia"). 8 mm = ~4 mm de translacao.
constexpr float GIRO_SUAVE_FIM_SYNC_MAX_MM = 8.0f;
// V5.2: TRILHA EFETIVA APRENDIDA. O giro principal e por encoder e usa a
// distancia entre rodas para converter 90 graus em arco de roda. Com
// ENTRE_RODAS_MM nominal e o escorregamento real, o robo girava de 80 a 110
// graus. Depois de cada giro, com o robo parado, a IMU mede o angulo REAL e o
// firmware corrige a trilha usada no giro seguinte. E o "90 graus" que se
// calibra sozinho, com o escorregamento do piso embutido.
constexpr float GIRO_TRILHA_ALFA = 0.5f;          // peso da medida nova
constexpr float GIRO_TRILHA_MIN_MM = 60.0f;       // fora disso a medida e descartada
constexpr float GIRO_TRILHA_MAX_MM = 160.0f;
constexpr float GIRO_TRILHA_ANGULO_MIN_GRAUS = 45.0f;  // so aprende com giro "de verdade"
constexpr float GIRO_TRILHA_ANGULO_MAX_GRAUS = 135.0f;
constexpr float GIRO_SUAVE_META_TOLERANCIA_MM = 0.7f;


// ===== V4.4 - BANCADA VL53L0X, SEM NAVEGACAO AUTOMATICA =====
// Identificacao fisica CONFIRMADA pelo usuario: U2 esquerda, U3 frente, U4 direita.
constexpr uint8_t LASER_XSHUT_U2 = 18; // ESQUERDO
constexpr uint8_t LASER_XSHUT_U3 = 19; // FRONTAL
constexpr uint8_t LASER_XSHUT_U4 = 23; // DIREITO
constexpr uint8_t LASER_ADDR_U2 = 0x30;
constexpr uint8_t LASER_ADDR_U3 = 0x31;
constexpr uint8_t LASER_ADDR_U4 = 0x32;
// Offset de calibracao por sensor, em mm: quanto a leitura fica ACIMA da
// distancia real. E subtraido antes do filtro e da deteccao de parede.
// Medidos na bancada com a nova eletronica (issue #151). O teste 3.2 do
// docs/7.3 mostrou que o erro muda acima de 150 mm; repetir com gabarito.
constexpr int LASER_OFFSET_U2_MM = 20; // ESQUERDO
constexpr int LASER_OFFSET_U3_MM = 20; // FRONTAL
constexpr int LASER_OFFSET_U4_MM = 15; // DIREITO
constexpr uint16_t LASER_MIN_DIAGNOSTICO_MM = 20;   // abaixo disso: INVALIDO, nao obstaculo livre
constexpr uint16_t LASER_MAX_DIAGNOSTICO_MM = 2000;
constexpr uint32_t LASER_ORCAMENTO_MEDICAO_US = 50000;
constexpr uint16_t LASER_FOLGA_PERIODO_MS = 10;
constexpr uint8_t LASER_CICLOS_VALIDADE = 4;
constexpr float LASER_EMA_ALFA = 0.50f;
constexpr int LASER_PAREDE_PRESENTE_MM = 120;
constexpr int LASER_PAREDE_AUSENTE_MM = 160;
constexpr uint32_t LASER_CAPTURE_TIMEOUT_MS = 20000;
constexpr uint16_t LASER_CAPTURE_MAX_AMOSTRAS = 120;
constexpr float LARGURA_CHASSI_ALVO_MM = 110.0f; // hipotese futura, nao largura medida atual


// ===== V4.5 - MOVIMENTO RETO (ENCODERS, SEM CORRECAO LASER) =====
// Limites iniciais de bancada: distancia curta e RPM baixos.
constexpr float RETO_DISTANCIA_MIN_MM = 30.0f;
constexpr float RETO_DISTANCIA_MAX_MM = 10000.0f; // limite fisico de teste: 10 m, nao infinito
constexpr float RETO_LIMITE_ENSAIO_CURTO_MM = 250.0f;
constexpr uint32_t RETO_TEMPO_MAX_LONGO_MS = 420000; // 7 minutos
// V4.6: sensor frontal U3 nao calibrado; limiar minimo CONSERVADOR,
// contado desde a janela optica do laser (nao ate a extremidade frontal).
constexpr uint16_t FRENTE_LIMIAR_MIN_MM = 200;
constexpr uint16_t FRENTE_LIMIAR_MAX_MM = 1000;
constexpr uint16_t FRENTE_LIMIAR_PADRAO_MM = 250;
constexpr uint32_t FRENTE_IDADE_MAX_MS = 150;
constexpr float FRENTE_RPM_MAX_TESTE = 35.0f;
// Com centralizacao a protecao frontal e obrigatoria. O valor fixo de 50 mm
// ficava ABAIXO da leitura do U3 com o robo encostado (ROBO_U3_TOCA_MM = 60):
// o robo batia antes de a protecao disparar. Agora e sempre o toque + folga.
constexpr uint16_t FRENTE_LIMIAR_CENTRO_MM = (uint16_t)(ROBO_U3_TOCA_MM + 15.0f);
// Corredor sem parede ao alcance do U3: partir exige essas leituras "sem alvo" seguidas.
constexpr uint8_t FRENTE_SEM_ALVO_PARTIDA = 3;
constexpr uint32_t FRENTE_MEMORIA_PARTIDA_MS = 500;
// 250 ms de reacao + desaceleracao aproximada: apenas margem EXPERIMENTAL.
// Esta previsao nao comprova distancia de frenagem; medir com regua.
constexpr float FRENTE_REACAO_S = 0.25f;
constexpr float FRENTE_DESACEL_ESTIMADA_MM_S2 = 200.0f;
constexpr float RETO_RPM_MIN = 20.0f;
constexpr float RETO_RPM_MAX = 60.0f;
constexpr float RETO_META_TOL_MM = 0.7f;
constexpr float RETO_SYNC_K_RPM_POR_MM = 3.0f;
constexpr float RETO_SYNC_CORRECAO_MAX_RPM = 8.0f;
constexpr float RETO_SYNC_FRAC_MAX = 0.30f;
constexpr float RETO_SYNC_TAXA_RPM_S = 70.0f;
constexpr float RETO_SYNC_FALHA_MM = 7.0f;
constexpr uint32_t RETO_SYNC_FALHA_MS = 300;
constexpr float RETO_SYNC_LIMITE_CHEGADA_MM = 2.5f;
constexpr float RETO_MARGEM_CURSO_MM = 12.0f;

// Centralização no corredor (#155). Medir na bancada com o robô centrado.
constexpr float CENTRO_ASSIMETRIA_MM = 0.0f;   // leitura E - leitura D centrado
constexpr float CENTRO_REF_FIXA_MM = -1.0f;    // leitura de um lado centrado; -1 = só a aprendida com duas paredes
constexpr int CENTRO_PAREDE_MAX_MM = LASER_PAREDE_PRESENTE_MM;
constexpr uint32_t CENTRO_IDADE_MAX_MS = 150;
constexpr float CENTRO_SALTO_MAX_MM = 15.0f;
constexpr float CENTRO_KP = 0.4f;
constexpr float CENTRO_KI_POR_MM = 0.004f;
constexpr float CENTRO_INTEGRAL_MAX_MM = 8.0f;
constexpr float CENTRO_ALVO_MAX_MM = 12.0f;
constexpr float CENTRO_TAXA_MM_S = 20.0f;


// Integracao 4.7: protecao frontal ainda NAO homologada na nova posicao.
// O firmware nao deve considerar leitura invalida como caminho livre.

// ===== Protecao da bateria LiPo 2S (battery.cpp / power_logic.h) =====
// Corte travado: abaixo de BAT_V_CORTE por BAT_TEMPO_CONFIRMACAO_MS, os motores
// param em qualquer modo e novos testes ficam bloqueados.
constexpr uint8_t INA219_ADDR = 0x40;
constexpr int BAT_CELULAS = 2;
constexpr float BAT_V_ALERTA = 7.0f;
constexpr float BAT_V_CORTE = 6.6f;
constexpr uint32_t BAT_TEMPO_CONFIRMACAO_MS = 2000;
constexpr float I_SOBRECORRENTE_MA = 2000.0f;   // somente aviso
constexpr uint32_t I_SOBRECORRENTE_TEMPO_MS = 500;
// Faixa dos limites temporarios de ensaio (POST /api/bateria/limites).
constexpr float BAT_V_CORTE_MAX_ENSAIO = 9.0f;
constexpr float I_SOBRECORRENTE_MIN_ENSAIO_MA = 50.0f;

// Envio remoto via HTTP para o backend. So ha tentativa de envio com algum
// aparelho conectado na rede do robo. O percentual de bateria usa BAT_CELULAS;
// sem leitura do INA219 nenhum POST e enviado, para nao exibir 0% falso.
constexpr bool TELEMETRIA_REMOTA_HABILITADA = true;
constexpr char TELEMETRIA_HOST[] = "192.168.4.2"; // computador conectado ao AP
constexpr uint16_t TELEMETRIA_PORTA = 8000;
constexpr char TELEMETRIA_CAMINHO[] = "/api/telemetry";
constexpr uint32_t TELEMETRIA_PERIODO_MS = 500; // 2 Hz; nao interfere no PI
constexpr uint32_t TELEMETRIA_TIMEOUT_MS = 150;

// ===== V4.8: NAVEGACAO AUTONOMA (EXIGE CALIBRACAO FISICA) =====
// Acionar somente DEPOIS dos testes de eletronica, laser e deslocamento no chao.
#if defined(NAV_HOST_SIM) && !defined(ARDUINO)
// SOMENTE executavel de testes nativos com sensores e ponte H FICTICIOS.
constexpr bool NAV_AUTONOMIA_HABILITADA = true;
constexpr bool NAV_LATERAIS_CALIBRADOS = true;
constexpr float NAV_U3_ATE_NARIZ_MM = 10.0f;
#else
constexpr bool NAV_AUTONOMIA_HABILITADA = true; // mudar para true apos checklist
constexpr bool NAV_LATERAIS_CALIBRADOS = true; // confirmar offsets e posicionamento
// Distancia da janela do LASER U3 ate o ponto MAIS AVANCADO do chassi (mm).
// Valor negativo impede armar: medir com regua no robo novo. Nunca chutar.
constexpr float NAV_U3_ATE_NARIZ_MM = ROBO_U3_TOCA_MM;
#endif
constexpr float NAV_CELULA_MM = 180.0f; // distancia entre centros: aferir no labirinto real
// V5.1: 25 -> 35 RPM (~62 mm/s). E o teto atual de FRENTE_RPM_MAX_TESTE; acima
// disso a reta e recusada. Mais RPM tambem eleva o feedforward de PWM.
constexpr float NAV_RPM_RETO = 35.0f;
// V5.1: 25 -> 30 RPM. Subida menor que a da reta: um dos erros vistos foi o
// giro passando do alvo, e mais velocidade no giro aumenta a ultrapassagem.
constexpr float NAV_RPM_GIRO = 30.0f;
constexpr float NAV_FOLGA_FRONTAL_MM = 15.0f;
constexpr uint32_t NAV_IDADE_SENSOR_MS = 160;
constexpr uint32_t NAV_TEMPO_SENSORES_MS = 2000;
constexpr uint32_t NAV_TEMPO_MISSAO_MS = 600000;
constexpr uint16_t NAV_MAX_PASSOS = 512; // limite de seguranca, nao garantia de completar qualquer mapa

// V4.9: opcoes novas conservadoras. Nao altera leitura/offset dos lasers.
// O status 2 continua inconclusivo por padrao; nunca confundir erro I2C com passagem.
// V4.9.1: habilitado. Sem isso, corredor aberto (VL53L0X sem retorno) nunca era
// confirmado como passagem e a missao parava com "sem_passagem_confirmada".
// Erro de I2C/timeout continuam sem liberar passagem.
constexpr bool NAV_SEM_ALVO_COMO_PASSAGEM = true;
constexpr uint8_t NAV_SEM_ALVO_AMOSTRAS_MINIMAS = 3;
#if defined(NAV_HOST_SIM) && !defined(ARDUINO)
constexpr bool NAV_RE_HABILITADA = true;
constexpr bool NAV_CENTRALIZAR_HABILITADA = true;
constexpr bool NAV_AJUSTE_ABERTURA_HABILITADO = true;
constexpr float NAV_ABERTURA_LARGURA_MM = 168.0f;
constexpr float NAV_U2_A_EIXO_MM = 0.0f;
constexpr float NAV_U4_A_EIXO_MM = 0.0f;
#else
// Sem sensor traseiro: re somente em ambiente controlado e apos validacao presencial.
constexpr bool NAV_RE_HABILITADA = false;
// V4.9.3: ligados por padrao. A parada passa a ser guiada pela borda da parede
// (ver NAV_SENSOR_LATERAL_AO_CENTRO_MM) e a centralizacao corrige o desvio lateral.
constexpr bool NAV_CENTRALIZAR_HABILITADA = true;
constexpr bool NAV_AJUSTE_ABERTURA_HABILITADO = true;
// Largura livre do corredor. NAO e mais o valor nominal do projeto (168 mm):
// e a largura que o proprio robo mediu nos dois ensaios de borda, ja com o
// escorregamento das rodas embutido. Durante a missao ele continua refinando
// a medida com os dois lasers laterais.
constexpr float NAV_ABERTURA_LARGURA_MM = 2.0f * ROBO_MEIA_ABERTURA_MM;
// Medida do ROBO (nao do labirinto): quanto os lasers laterais ficam a FRENTE
// do eixo. Derivado dos ensaios de borda, nao mais estimado.
constexpr float NAV_U2_A_EIXO_MM = ROBO_LATERAL_AO_EIXO_MM;
constexpr float NAV_U4_A_EIXO_MM = ROBO_LATERAL_AO_EIXO_MM;
#endif
// V4.9.3: medida do ROBO: distancia lateral entre a janela de cada laser
// lateral e a linha central do robo (mm). Com ela o robo calcula a largura do
// corredor sozinho: largura = leituraE + leituraD + 2*este valor.
// -1 = ainda nao medido: usa NAV_ABERTURA_LARGURA_MM como largura.
constexpr float NAV_SENSOR_LATERAL_AO_CENTRO_MM = -1.0f;
// Desvio lateral medido parado na celula: acima deste valor o robo sai da celula
// levemente inclinado para voltar ao centro durante a reta seguinte.
constexpr float NAV_DESVIO_MIN_MM = 6.0f;
constexpr float NAV_DESVIO_MAX_GRAUS = 10.0f;
constexpr float NAV_RPM_RE = 20.0f;
// V4.9.3: a parada pela borda da parede pode corrigir ate 40 mm a distancia
// nominal da celula (antes 12 mm, pouco para o labirinto de teste).
constexpr float NAV_AJUSTE_MAX_MM = 40.0f;
constexpr float NAV_AJUSTE_RESTANTE_MIN_MM = 15.0f;
constexpr float NAV_YAW_RETO_MAX_GRAUS = 12.0f;
constexpr float NAV_YAW_GIRO_TOL_GRAUS = 12.0f;
// ===== V4.9.2: RUMO ABSOLUTO PELA IMU =====
// O giro de 90 por encoder depende de ENTRE_RODAS_MM (valor nominal) e do
// escorregamento; o erro de cada giro se ACUMULAVA porque o reto seguinte so
// mantinha o rumo em que comecou. Agora a navegacao guarda o rumo esperado
// (yaw inicial + 90 por giro) e: (1) depois de cada giro corrige pela IMU ate
// o rumo esperado; (2) antes de cada reta, se o erro passar de
// NAV_ALINHAR_GRAUS, faz um giro curto de alinhamento.
constexpr bool NAV_GIRO_CORRIGIR_IMU = true;
constexpr float NAV_ALINHAR_GRAUS = 2.0f;
constexpr float NAV_ALINHAR_MAX_GRAUS = 15.0f;
constexpr uint8_t NAV_ALINHAR_TENTATIVAS = 2;
// OBSOLETO na V5.0: substituido pelo reposicionamento calculado em
// nav_pose_logic.h (navPoseAjusteAntesDoGiroMm), que leva o eixo ao centro da
// abertura em vez de recuar um valor fixo. Mantido so para referencia.
// Recuo ANTES de um giro de 90 graus (mm), so quando a celula de tras ja foi
// percorrida. Serve para robo com eixo na traseira: o nariz varre um circulo
// de raio R em volta do eixo; se R for maior que a metade livre do corredor,
// o nariz bate na parede da frente durante o giro. Recuo sugerido:
//   R_medido + 5 mm - (largura livre do corredor / 2)
// 0 = desligado. Faixa aceita 0..40 mm. Ajustar por ensaio.
constexpr float NAV_GIRO_RECUO_MM = 0.0f;
constexpr float NAV_GIRO_RECUO_RPM = 30.0f; // V5.1: usado pelo reposicionamento antes do giro
// V4.9.1: tempo maximo com o U3 sem evidencia durante o avanco antes de parar
// (uma unica amostra ruim nao interrompe mais a missao).
constexpr uint32_t NAV_FRENTE_INDEFINIDA_MAX_MS = 300;

// ===== V5.0: POSICAO LONGITUDINAL DENTRO DA CELULA (nav_pose_logic.h) =====
// O robo deixou de supor que toda reta comeca e termina no centro da celula.
// Ele guarda o deslocamento do eixo em relacao ao centro e desconta na reta
// seguinte, de modo que uma parada antecipada nao acumula erro nem invalida
// a pose.
// Missao que comeca com a TRASEIRA ENCOSTADA na parede (e o caso real: o robo
// nao e colocado no centro do quadrado). Com false, parte do centro.
constexpr bool NAV_PARTIDA_TRASEIRA_ENCOSTADA = true;
// Folga que o robo mantem da parede da frente ao terminar uma reta (leitura do
// U3 = ROBO_U3_TOCA_MM + esta folga).
// V5.2: 15 -> 5 mm. O robo gira encostado na parede (a parte que excede passa
// por cima dos 5 cm), e quanto mais longe dela ele gira, mais erro leva para a
// celula seguinte. 5 mm cobre o ruido do VL53L0X nessa distancia (~3 mm) sem
// empurrar a parede, o que travaria as rodas e cairia em "sem_movimento".
constexpr float NAV_PARADA_FOLGA_MM = 5.0f;
// Margem dura: abaixo de ROBO_U3_TOCA_MM + isto, para na hora (emergencia).
// V5.2: 5 -> 2 mm, para ficar abaixo da nova parada de trabalho.
constexpr float NAV_EMERGENCIA_FOLGA_MM = 2.0f;
// Encurtar a meta so vale a pena se ainda sobra este tanto para desacelerar.
constexpr float NAV_ENCURTAR_MIN_RESTANTE_MM = 12.0f;
// Reposicionamento longitudinal antes de um giro de 90 graus.
constexpr float NAV_GIRO_OFFSET_TOL_MM = 5.0f;
constexpr float NAV_GIRO_OFFSET_MAX_MM = 55.0f;
// Offset fora desta margem alem da meia abertura = odometria perdida.
constexpr float NAV_OFFSET_MARGEM_MM = 35.0f;
