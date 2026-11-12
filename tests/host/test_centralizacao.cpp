// Centralização (#155): corredor simulado, com posição lateral e ângulo
// calculados das rodas e lasers a cada 60 ms com ruído. Não substitui o chão.
#include "motion_control.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "config.h"
#include "lasers_logic.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

static uint32_t t = 1000;
uint32_t millis() { return t; }
static DadosEncoder encoder{0, 0, 0, 0};
DadosEncoder lerEncoders() { return encoder; }
DadosIMU lerIMU() { DadosIMU d; return d; }
bool zerarYawIMU() { return true; }

static const float CPR_E = 815.55f, CPR_D = 815.48f;
static const float CIRC = 3.14159265f * DIAMETRO_RODA_MM;
static const float CORREDOR_MM = 168.0f;   // célula de 180 mm, parede de 12 mm
static const float JANELA_MM = 35.0f;      // janela do laser até o centro do robô
static const float MEIA_LARGURA_MM = 50.0f;

struct Roda { int pwm = 0; bool frente = true; float rpm = 0, resto = 0; bool presa = false; };
static Roda rE, rD;
void setMotorEsq(int p, bool f) { rE.pwm = p; rE.frente = f; }
void setMotorDir(int p, bool f) { rD.pwm = p; rD.frente = f; }
void frearMotorEsq() { rE.pwm = 0; }
void frearMotorDir() { rD.pwm = 0; }

static int64_t passoRoda(Roda &r, float cpr, float &cps) {
  float alvo = r.presa ? 0.0f : fmaxf(0.0f, (float)(r.pwm - 12)) * 0.9f;
  r.rpm += (alvo - r.rpm) * 0.35f;
  cps = (r.frente ? 1 : -1) * r.rpm * cpr / 60.0f;
  r.resto += cps * 0.02f;
  const int64_t i = (int64_t)std::trunc(r.resto);
  r.resto -= (float)i;
  return i;
}

struct Cenario {
  const char *nome;
  float x0 = 0, theta0Graus = 0;
  float escalaD = 1.0f;          // roda direita "maior": anda mais por pulso
  float semDireitaDe = -1, semDireitaAte = -1;
  float semParedesDe = -1, semParedesAte = -1;
  float laserTravaEm = -1;       // a partir daqui o laser não atualiza mais
  float bordaDireitaEm = -1;     // parede direita some em rampa (49 -> 75 -> 110 -> 400)
  float picoEm = -1;             // uma leitura isolada errada na esquerda
  bool rodaPresa = false;
  bool centralizar = true;
  float distancia = 900;
};

struct Resultado { std::string motivo; float xFinal = 0, xMax = 0; ModoCentro ultimoModo = ModoCentro::NENHUMA; };

static float ruido() { return (float)(std::rand() % 5 - 2); }

static Resultado rodar(const Cenario &c) {
  std::srand(7);
  desativarControleMovimento();
  rE = Roda{}; rD = Roda{}; rD.presa = c.rodaPresa;
  encoder = DadosEncoder{0, 0, 0, 0};
  float x = c.x0, th = c.theta0Graus * 3.14159265f / 180.0f, s = 0;
  FiltroLaser fE(LASER_EMA_ALFA), fD(LASER_EMA_ALFA);
  DetectorParede pE(LASER_PAREDE_PRESENTE_MM, LASER_PAREDE_AUSENTE_MM), pD(LASER_PAREDE_PRESENTE_MM, LASER_PAREDE_AUSENTE_MM);
  LeitorLateral lE(CENTRO_PAREDE_MAX_MM, CENTRO_IDADE_MAX_MS, CENTRO_SALTO_MAX_MM);
  LeitorLateral lD(CENTRO_PAREDE_MAX_MM, CENTRO_IDADE_MAX_MS, CENTRO_SALTO_MAX_MM);
  int filtE = -1, filtD = -1; EstadoParede parE = PAREDE_INDEFINIDA, parD = PAREDE_INDEFINIDA;
  uint32_t ultimaE = 0, ultimaD = 0, amostra = 0, proximoLaser = t;
  int passoBorda = 0;
  bool picoFeito = false;
  Resultado r;
  if (!iniciarControleReto(c.distancia, 30, CPR_E, CPR_D, true, c.centralizar)) { r.motivo = "nao_iniciou"; return r; }
  for (int i = 0; i < 4000; i++) {
    const int64_t pe = passoRoda(rE, CPR_E, encoder.cpsEsq), pd = passoRoda(rD, CPR_D, encoder.cpsDir);
    encoder.esq += pe; encoder.dir += pd;
    const float dE = pe * CIRC / CPR_E, dD = pd * CIRC / CPR_D * c.escalaD;
    th += (dE - dD) / ENTRE_RODAS_MM;
    x += 0.5f * (dE + dD) * sinf(th);
    s += 0.5f * (dE + dD) * cosf(th);
    t += 20;
    r.xMax = fmaxf(r.xMax, fabsf(x));
    const bool travado = c.laserTravaEm >= 0 && s >= c.laserTravaEm;
    if (!travado && t >= proximoLaser) {
      proximoLaser = t + 60;
      const bool semTodas = s >= c.semParedesDe && s < c.semParedesAte;
      const bool semDir = semTodas || (s >= c.semDireitaDe && s < c.semDireitaAte);
      float leE = semTodas ? 400 : (x + CORREDOR_MM / 2 - JANELA_MM) / cosf(th) + ruido();
      float leD = semDir ? 400 : (CORREDOR_MM / 2 - x - JANELA_MM) / cosf(th) + ruido();
      if (c.bordaDireitaEm >= 0 && s >= c.bordaDireitaEm) {
        static const float rampa[] = {75, 110, 400};
        leD = rampa[passoBorda < 2 ? passoBorda : 2];
        ++passoBorda;
      }
      if (c.picoEm >= 0 && s >= c.picoEm && !picoFeito) { leE += 40; picoFeito = true; }
      ++amostra;
      filtE = fE.atualizar((int)leE); parE = pE.atualizar(filtE); ultimaE = t;
      filtD = fD.atualizar((int)leD); parD = pD.atualizar(filtD); ultimaD = t;
    }
    // Mesma validação do web_telemetry.
    informarLateraisReto(lE.atualizar(amostra, filtE, parE == PAREDE_PRESENTE, t - ultimaE),
                         lD.atualizar(amostra, filtD, parD == PAREDE_PRESENTE, t - ultimaD));
    const char *m = atualizarControleMovimento();
    r.ultimoModo = lerControleMovimento().centroModo;
    if (m) { r.motivo = m; break; }
  }
  if (r.motivo.empty()) r.motivo = "timeout";
  r.xFinal = x;
  return r;
}

static int falhas = 0;
static void conferir(bool ok, const char *d) { std::printf("%s %s\n", ok ? "ok  " : "FALHA", d); if (!ok) ++falhas; }
static Resultado mostrar(const Cenario &c) {
  Resultado r = rodar(c);
  std::printf("  %-34s %-30s x final=%6.1f mm  x max=%5.1f mm\n", c.nome, r.motivo.c_str(), r.xFinal, r.xMax);
  return r;
}

int main() {
  const float encosta = CORREDOR_MM / 2 - MEIA_LARGURA_MM;
  std::printf("  (encosta na parede com |x| > %.0f mm)\n", encosta);

  Cenario torto{"saida torta +3 graus"}; torto.theta0Graus = 3;
  Cenario semCentro = torto; semCentro.nome = "saida torta, centralizacao desligada"; semCentro.centralizar = false;
  Resultado a = mostrar(torto), a0 = mostrar(semCentro);
  conferir(a.motivo == "reto_meta_atingida" && a.xMax < encosta, "torto: chega sem encostar");
  conferir(fabsf(a.xFinal) < 5, "torto: termina a ate 5 mm do centro");
  conferir(a0.xMax > a.xMax, "torto: sem centralizacao desvia mais");

  Cenario tortoE{"saida torta -3 graus"}; tortoE.theta0Graus = -3;
  Resultado b = mostrar(tortoE);
  conferir(b.motivo == "reto_meta_atingida" && fabsf(b.xFinal) < 5, "torto para o outro lado: volta ao centro");

  Cenario desl{"saida deslocada +15 mm"}; desl.x0 = 15;
  Cenario deslE{"saida deslocada -15 mm"}; deslE.x0 = -15;
  Resultado c1 = mostrar(desl), c2 = mostrar(deslE);
  conferir(c1.motivo == "reto_meta_atingida" && fabsf(c1.xFinal) < 5, "deslocada para a direita: volta ao centro");
  conferir(c2.motivo == "reto_meta_atingida" && fabsf(c2.xFinal) < 5, "deslocada para a esquerda: volta ao centro");

  Cenario roda{"roda direita 1% maior"}; roda.escalaD = 1.01f;
  Cenario roda0 = roda; roda0.nome = "roda 1% maior, desligada"; roda0.centralizar = false;
  Resultado d1 = mostrar(roda), d0 = mostrar(roda0);
  conferir(d1.motivo == "reto_meta_atingida" && d1.xMax < encosta && fabsf(d1.xFinal) < 5, "roda maior: corrige o desvio");
  conferir(d0.xMax > d1.xMax, "roda maior: sem centralizacao desvia mais");

  Cenario umaParede{"sem parede direita 360..720 mm"}; umaParede.x0 = 10;
  umaParede.semDireitaDe = 360; umaParede.semDireitaAte = 720; umaParede.distancia = 1080;
  Resultado e = mostrar(umaParede);
  conferir(e.motivo == "reto_meta_atingida" && e.xMax < encosta && fabsf(e.xFinal) < 5, "uma parede: continua no centro");

  Cenario abertura{"sem paredes 360..540 mm, torto"}; abertura.theta0Graus = 2;
  abertura.semParedesDe = 360; abertura.semParedesAte = 540;
  Resultado f = mostrar(abertura);
  conferir(f.motivo == "reto_meta_atingida" && f.xMax < encosta, "abertura: atravessa sem encostar");

  Cenario trava{"laser trava em 300 mm"}; trava.theta0Graus = 2; trava.laserTravaEm = 300;
  Resultado g = mostrar(trava);
  conferir(g.motivo == "reto_meta_atingida" && g.ultimoModo == ModoCentro::NENHUMA, "laser travado: segue so pelos encoders");

  Cenario borda{"parede direita some em rampa"}; borda.bordaDireitaEm = 400;
  Resultado bo = mostrar(borda);
  conferir(bo.motivo == "reto_meta_atingida" && bo.xMax < 5, "borda gradual: nao puxa o robo");

  Cenario pico{"pico isolado na esquerda"}; pico.picoEm = 300;
  Resultado pi = mostrar(pico);
  conferir(pi.motivo == "reto_meta_atingida" && pi.xMax < 5, "pico isolado: nao puxa o robo");

  Cenario fim{"paredes somem nos ultimos 15 mm"}; fim.x0 = 15; fim.escalaD = 1.01f;
  fim.semParedesDe = 885; fim.semParedesAte = 2000;
  Resultado fi = mostrar(fim);
  conferir(fi.motivo == "reto_meta_atingida", "paredes somem no fim: chega sem dessincronia");

  Cenario presa{"roda direita presa"}; presa.rodaPresa = true;
  Resultado h = mostrar(presa);
  conferir(h.motivo != "reto_meta_atingida" && h.motivo != "timeout", "roda presa: o movimento e interrompido");

  desativarControleMovimento();
  conferir(!iniciarControleReto(-200, 30, CPR_E, CPR_D, true, true), "recusa centralizacao de re");
  conferir(!iniciarControleReto(200, 30, CPR_E, CPR_D, false, true), "recusa centralizacao sem sincronismo");

  std::printf(falhas ? "FALHOU: %d\n" : "PASS: centralizacao no corredor simulada\n", falhas);
  return falhas ? 1 : 0;
}
