#pragma once
#include <Arduino.h>

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
constexpr uint32_t PARTIDA_AJUDA_ATE_MS = 850;
constexpr float KP_RPM = 0.40f;                 // parametros experimentais
constexpr float KI_RPM = 1.25f;                 // PWM/(RPM*s)
constexpr float FF_PWM_BASE = 8.0f;
constexpr float FF_PWM_POR_RPM = 0.44f;
constexpr float LIMITE_INTEGRAL_PWM = 70.0f;
constexpr int POSICAO_TOLERANCIA_CONTAGENS = 4;
constexpr int POSICAO_META_MAX_CONTAGENS = 2000;
constexpr uint32_t CONTROLE_STALL_MS = 1200;
constexpr uint32_t FREIO_MEDICAO_FINAL_MS = 1000;

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

// ===== V4.3 - GIRO COM SINCRONIZACAO SUAVE =====
// Correcao proporcional limitada; nao freia individualmente uma roda.
// Valores conservadores para testes com o centro do eixo marcado no chao.
constexpr float GIRO_SUAVE_K_RPM_POR_MM = 3.0f;
constexpr float GIRO_SUAVE_CORRECAO_FRAC_MAX = 0.30f;
constexpr float GIRO_SUAVE_CORRECAO_RPM_MAX = 8.0f;
constexpr float GIRO_SUAVE_TAXA_CORRECAO_RPM_S = 75.0f;
constexpr float GIRO_SUAVE_DESACEL_INICIO_MM = 18.0f;
constexpr float GIRO_SUAVE_RPM_PERTO_META = 25.0f;
constexpr float GIRO_SUAVE_FALHA_MM = 6.0f;
constexpr uint32_t GIRO_SUAVE_FALHA_MS = 250;
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
