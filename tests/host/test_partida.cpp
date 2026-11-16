// Partida conjunta (#152): cada roda tem um atrito estático diferente, então
// uma delas precisa da ajuda de partida e a outra não. Simulação grosseira,
// não substitui o ensaio no chão.
#include "motion_control.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "config.h"
#include <cmath>
#include <cstdio>
#include <string>

static uint32_t t = 1000;
uint32_t millis() { return t; }
static DadosEncoder encoder{0, 0, 0, 0};
DadosEncoder lerEncoders() { return encoder; }
DadosIMU lerIMU() { DadosIMU d; return d; }
bool zerarYawIMU() { return true; }

static const float CPR_E = 815.55f, CPR_D = 815.48f;
static const float CIRC = 3.14159265f * DIAMETRO_RODA_MM;

struct Roda {
  int pwm = 0;
  bool frente = true;
  int estatico = 0;    // PWM para sair do repouso
  int cinetico = 20;   // abaixo disso a roda para
  bool andando = false;
  float rpm = 0;
  float resto = 0;     // fração de contagem acumulada
};
static Roda rodaE, rodaD;

void setMotorEsq(int pwm, bool frente) { rodaE.pwm = pwm; rodaE.frente = frente; }
void setMotorDir(int pwm, bool frente) { rodaD.pwm = pwm; rodaD.frente = frente; }
void frearMotorEsq() { rodaE.pwm = 0; }
void frearMotorDir() { rodaD.pwm = 0; }

static int64_t passo(Roda &r, float cpr, float &cps) {
  if (!r.andando && r.pwm >= r.estatico) r.andando = true;
  float alvo = r.andando ? fmaxf(0.0f, (float)(r.pwm - r.cinetico)) * 0.9f : 0.0f;
  r.rpm += (alvo - r.rpm) * 0.35f;  // inércia de primeira ordem
  if (r.andando && r.pwm < r.cinetico && r.rpm < 1.0f) { r.andando = false; r.rpm = 0; }
  const float sinal = r.frente ? 1.0f : -1.0f;
  cps = sinal * r.rpm * cpr / 60.0f;
  r.resto += cps * 0.02f;
  const int64_t inteiro = (int64_t)std::trunc(r.resto);
  r.resto -= (float)inteiro;
  return inteiro;
}

static void simular() {
  encoder.esq += passo(rodaE, CPR_E, encoder.cpsEsq);
  encoder.dir += passo(rodaD, CPR_D, encoder.cpsDir);
  t += 20;
}

static void reiniciarRodas(int estaticoE, int estaticoD) {
  rodaE = Roda{};
  rodaD = Roda{};
  rodaE.estatico = estaticoE;
  rodaD.estatico = estaticoD;
  encoder = DadosEncoder{0, 0, 0, 0};
}

struct Resultado {
  std::string motivo;
  float difMaxMm = 0;   // medido pelo teste, direto das contagens
  ControleInfo info;
};

static Resultado rodarReto(float mm, int estaticoE, int estaticoD) {
  desativarControleMovimento();
  reiniciarRodas(estaticoE, estaticoD);
  Resultado r;
  if (!iniciarControleReto(mm, 40, CPR_E, CPR_D, true)) { r.motivo = "nao_iniciou"; return r; }
  for (int i = 0; i < 1500; i++) {
    simular();
    const float e = fabsf((float)encoder.esq) * CIRC / CPR_E;
    const float d = fabsf((float)encoder.dir) * CIRC / CPR_D;
    if (0.5f * (e + d) <= PARTIDA_JANELA_MM) r.difMaxMm = fmaxf(r.difMaxMm, fabsf(e - d));
    const char *m = atualizarControleMovimento();
    if (m) { r.motivo = m; break; }
  }
  if (r.motivo.empty()) r.motivo = "timeout";
  r.info = lerControleMovimento();
  return r;
}

static int falhas = 0;
static void conferir(bool ok, const char *descricao) {
  std::printf("%s %s\n", ok ? "ok  " : "FALHA", descricao);
  if (!ok) ++falhas;
}

int main() {
  // 1) Esquerda sai fácil, direita precisa da ajuda: diferença nos primeiros 50 mm.
  Resultado a = rodarReto(200, 25, 60);
  std::printf("  reto E25/D60: motivo=%s difMax(teste)=%.2f mm difMax(firmware)=%.2f mm dif=%.2f atraso=%d ms\n",
              a.motivo.c_str(), a.difMaxMm, a.info.partida.diferencaMaxMm, a.info.partida.diferencaMm,
              (int)a.info.partida.atrasoMs());
  conferir(a.motivo == "reto_meta_atingida", "reto com atritos diferentes chega na meta");
  conferir(a.difMaxMm <= 2.0f, "diferenca nos primeiros 50 mm ate 2 mm (simulado)");
  conferir(a.info.partida.janelaCompleta, "registro fecha a janela de 50 mm");
  conferir(fabsf(a.info.partida.diferencaMaxMm - a.difMaxMm) < 0.5f, "registro do firmware bate com o teste");
  conferir(a.info.partida.duasPartiram(), "registro marca a saida das duas rodas");

  // 2) Mesmo caso invertido.
  Resultado b = rodarReto(200, 60, 25);
  std::printf("  reto E60/D25: motivo=%s difMax=%.2f mm atraso=%d ms\n",
              b.motivo.c_str(), b.difMaxMm, (int)b.info.partida.atrasoMs());
  conferir(b.motivo == "reto_meta_atingida", "caso invertido chega na meta");
  conferir(b.difMaxMm <= 2.0f, "caso invertido: diferenca ate 2 mm (simulado)");

  // 3) Direita precisa de mais PWM que a ajuda (65): a esquerda nao pode disparar.
  //    No firmware anterior esta simulacao dava 9,2 mm.
  Resultado forte = rodarReto(200, 15, 70);
  std::printf("  reto E15/D70: motivo=%s difMax=%.2f mm\n", forte.motivo.c_str(), forte.difMaxMm);
  conferir(forte.motivo == "reto_meta_atingida", "roda acima da ajuda: reto chega na meta");
  conferir(forte.difMaxMm < 6.0f, "roda acima da ajuda: a solta nao dispara (simulado)");

  // 4) Roda travada continua caindo no diagnostico de travamento.
  Resultado c = rodarReto(200, 25, 999);
  std::printf("  reto D travada: motivo=%s\n", c.motivo.c_str());
  conferir(c.motivo == "encoder_ou_motor_direito_sem_movimento" || c.motivo == "reto_dessincronia_limite",
           "roda travada interrompe o movimento");

  // 5) Em RPM nao ha limite de sincronismo: a roda travada cai no diagnostico
  //    de 1,2 s mesmo com a rampa segurada no piso.
  desativarControleMovimento();
  reiniciarRodas(25, 999);
  const char *travou = nullptr;
  if (iniciarControleRpm(40, 40, CPR_E, CPR_D))
    for (int i = 0; i < 500 && !travou; i++) { simular(); travou = atualizarControleMovimento(); }
  std::printf("  rpm D travada: motivo=%s\n", travou ? travou : "timeout");
  conferir(travou && std::string(travou) == "encoder_ou_motor_direito_sem_movimento",
           "rpm com roda travada cai no diagnostico de travamento");
  desativarControleMovimento();
  conferir(!lerControleMovimento().partidaConjunta, "parar o controle encerra a fase de partida");

  // 6) Posicao com uma roda so: a rampa nao fica presa no piso.
  desativarControleMovimento();
  reiniciarRodas(25, 25);
  const char *m = nullptr;
  bool passouDoPiso = false;
  if (iniciarControlePosicao(800, 0, 60, CPR_E, CPR_D)) {
    for (int i = 0; i < 1500 && !m; i++) {
      simular();
      m = atualizarControleMovimento();
      const ControleInfo ci = lerControleMovimento();
      if (ci.partidaConjunta) { m = "partida_conjunta_com_uma_roda"; break; }
      if (fabsf(ci.rampaE) > PARTIDA_RPM_PISO + 1) passouDoPiso = true;
    }
  }
  std::printf("  posicao so E: motivo=%s\n", m ? m : "timeout");
  conferir(m && std::string(m) == "posicao_meta_atingida", "posicao com uma roda conclui");
  conferir(passouDoPiso, "posicao com uma roda nao segura a rampa");

  std::printf(falhas ? "FALHOU: %d\n" : "PASS: partida conjunta simulada\n", falhas);
  return falhas ? 1 : 0;
}
