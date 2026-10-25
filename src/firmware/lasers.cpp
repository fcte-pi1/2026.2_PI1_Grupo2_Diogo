#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "lasers.h"
#include "config.h"

struct Laser {
  Adafruit_VL53L0X sensor;
  bool ativo = false;
  int ultimaLeitura = LASER_SEM_LEITURA;
  int ultimaBruta = LASER_SEM_LEITURA;
  EstadoParede estadoParede = PAREDE_INDEFINIDA;
  FiltroLaser filtro = FiltroLaser(LASER_FILTRO_ALFA);
  DetectorParede parede = DetectorParede(PAREDE_LIMIAR_COM_MM, PAREDE_LIMIAR_SEM_MM);
  uint32_t ultimaAtualizacaoMs = 0;
  uint32_t validadeMs = 0;
};

static Laser lasers[3];

static const uint8_t PINOS_XSHUT[3] = {XSHUT_ESQ, XSHUT_FTE, XSHUT_DIR};
static const uint8_t ENDERECOS[3]   = {ADDR_LASER_ESQ, ADDR_LASER_FTE, ADDR_LASER_DIR};
static const char *const NOMES[3]   = {"U2 (Esq)", "U3 (Fte)", "U4 (Dir)"};

static bool configurarModo(Adafruit_VL53L0X &sensor) {
  Adafruit_VL53L0X::VL53L0X_Sense_config_t modo;
  switch (LASER_MODO) {
    case LASER_MODO_LONG_RANGE:    modo = Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE; break;
    case LASER_MODO_ALTA_VELOC:    modo = Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED; break;
    case LASER_MODO_ALTA_PRECISAO: modo = Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY; break;
    default:                       modo = Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT; break;
  }
  if (!sensor.configSensor(modo)) return false;
  if (LASER_ORCAMENTO_US > 0 &&
      !sensor.setMeasurementTimingBudgetMicroSeconds(LASER_ORCAMENTO_US)) return false;
  return true;
}

static bool iniciarLaser(SensorLaser id) {
  Laser &l = lasers[id];

  digitalWrite(PINOS_XSHUT[id], HIGH);
  delay(15);
  if (l.sensor.begin(ENDERECOS[id]) && configurarModo(l.sensor)) {
    uint32_t orcamentoUs = l.sensor.getMeasurementTimingBudgetMicroSeconds();
    uint16_t periodoMs = periodoMedicaoMs(orcamentoUs, LASER_MARGEM_PERIODO_MS);
    if (l.sensor.Status == VL53L0X_ERROR_NONE && l.sensor.startRangeContinuous(periodoMs)) {
      l.validadeMs = validadeLeituraMs(periodoMs, LASER_CICLOS_VALIDADE);
      Serial.printf("[LASER] %s inicializado em 0x%02X (modo %d, orcamento %lu us, periodo %u ms)\n",
                    NOMES[id], ENDERECOS[id], LASER_MODO,
                    (unsigned long)orcamentoUs, periodoMs);
      return true;
    }
  }

  digitalWrite(PINOS_XSHUT[id], LOW);
  Serial.printf("[LASER] %s FALHOU (begin/inicio em 0x%02X); XSHUT desligado\n",
                NOMES[id], ENDERECOS[id]);
  return false;
}

void initLasers() {
  for (int i = 0; i < 3; i++) {
    pinMode(PINOS_XSHUT[i], OUTPUT);
    digitalWrite(PINOS_XSHUT[i], LOW);
  }
  delay(50);

  for (int i = 0; i < 3; i++) {
    lasers[i].ativo = iniciarLaser((SensorLaser)i);
    lasers[i].ultimaAtualizacaoMs = millis();
  }
}

static void atualizarLeitura(Laser &l) {
  l.ultimaLeitura = l.filtro.atualizar(l.ultimaBruta);
  l.estadoParede = l.parede.atualizar(l.ultimaLeitura);
}

int ler_distancia_mm(SensorLaser id) {
  if (id < LASER_ESQ || id > LASER_DIR) return LASER_SEM_LEITURA;
  Laser &l = lasers[id];
  if (!l.ativo) return LASER_SEM_LEITURA;

  uint32_t agora = millis();
  if (l.sensor.isRangeComplete()) {
    uint16_t mm = l.sensor.readRangeResult();
    bool erro = l.sensor.Status != VL53L0X_ERROR_NONE;
    l.ultimaBruta = classificarLeitura(erro, l.sensor.readRangeStatus(), mm,
                                       LASER_DIST_MIN_MM, LASER_DIST_MAX_MM);
    l.ultimaAtualizacaoMs = agora;
    atualizarLeitura(l);
  } else if (leituraVencida(agora, l.ultimaAtualizacaoMs, l.validadeMs)) {
    l.ultimaBruta = LASER_TIMEOUT;
    atualizarLeitura(l);
  }
  return l.ultimaLeitura;
}

int ler_distancia_bruta_mm(SensorLaser id) {
  ler_distancia_mm(id);
  return lasers[id].ultimaBruta;
}

EstadoParede obter_parede(SensorLaser id) {
  ler_distancia_mm(id);
  return lasers[id].estadoParede;
}

static int paraLegado(int leitura) {
  return leitura >= 0 ? leitura : -1;
}

void readLasers(int &distEsq, int &distFte, int &distDir) {
  distEsq = paraLegado(ler_distancia_mm(LASER_ESQ));
  distFte = paraLegado(ler_distancia_mm(LASER_FTE));
  distDir = paraLegado(ler_distancia_mm(LASER_DIR));
}
