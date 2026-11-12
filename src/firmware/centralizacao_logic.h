#pragma once

#include <math.h>
#include <stdint.h>

// Centralização no corredor pelos lasers laterais (#155).

enum class ModoCentro : uint8_t { NENHUMA, DUAS, ESQUERDA, DIREITA };

inline const char* textoModoCentro(ModoCentro m) {
  switch (m) {
    case ModoCentro::DUAS: return "DUAS";
    case ModoCentro::ESQUERDA: return "ESQUERDA";
    case ModoCentro::DIREITA: return "DIREITA";
    default: return "NENHUMA";
  }
}

struct LateralLida {
  bool valida = false;
  float mm = 0;
};

// Valida um laser lateral. Só avalia amostra nova; uma leitura só vale depois de
// duas seguidas sem salto (borda de parede, poste) e sem pausa do sensor.
class LeitorLateral {
public:
  LeitorLateral(int paredeMaxMm, uint32_t idadeMaxMs, float saltoMaxMm)
      : paredeMax(paredeMaxMm), idadeMax(idadeMaxMs), saltoMax(saltoMaxMm) {}

  void reiniciar() { temAmostra = false; seguidas = 0; aceita = false; }

  // amostra: contador de leituras do sensor; filtradoMm < 0 = inválida.
  LateralLida atualizar(uint32_t amostra, int filtradoMm, bool paredePresente, uint32_t idadeMs) {
    if (!temAmostra || amostra != ultimaAmostra) {
      temAmostra = true;
      ultimaAmostra = amostra;
      avaliar(filtradoMm, paredePresente);
    }
    if (idadeMs > idadeMax) { seguidas = 0; aceita = false; }
    LateralLida l;
    l.valida = aceita && idadeMs <= idadeMax;
    l.mm = valor;
    return l;
  }

private:
  void avaliar(int filtradoMm, bool paredePresente) {
    if (filtradoMm < 0 || !paredePresente || filtradoMm > paredeMax) {
      seguidas = 0;
      aceita = false;
      return;
    }
    const float mm = (float)filtradoMm;
    seguidas = (seguidas > 0 && fabsf(mm - valor) <= saltoMax) ? (seguidas < 2 ? seguidas + 1 : 2) : 1;
    valor = mm;
    aceita = seguidas >= 2;
  }

  int paredeMax;
  uint32_t idadeMax;
  float saltoMax;
  bool temAmostra = false;
  uint32_t ultimaAmostra = 0;
  uint8_t seguidas = 0;
  bool aceita = false;
  float valor = 0;
};

struct ParametrosCentro {
  float assimetriaMm = 0;        // leitura E - leitura D com o robô centrado
  float refFixaMm = -1;          // leitura de cada lado centrado; < 0 = só a aprendida
  float kp = 0.4f;               // mm de sincronismo por mm de erro lateral
  float kiPorMm = 0.004f;        // integral por mm de erro e por mm percorrido
  float integralMaxMm = 8.0f;
  float alvoMaxMm = 12.0f;
  float taxaMmPorS = 20.0f;      // variação máxima do alvo
  float aprendizado = 0.1f;      // peso de cada amostra na referência aprendida
};

// Erro lateral positivo: robô deslocado para a direita.
// Alvo: diferença de percurso E - D que o sincronismo do reto deve seguir.
class Centralizador {
public:
  explicit Centralizador(const ParametrosCentro& p) : par(p) {}

  void reiniciar() {
    modo_ = ModoCentro::NENHUMA;
    erro_ = NAN;
    integral = proporcional = alvo_ = 0;
    refAprendida = -1;
  }

  // percursoMm: quanto o robô andou desde a última chamada.
  float atualizar(LateralLida e, LateralLida d, float percursoMm, float dt) {
    escolherModo(e, d);
    float alvoProp = 0;
    if (modo_ != ModoCentro::NENHUMA) {
      integral = limitar(integral + par.kiPorMm * erro_ * fmaxf(percursoMm, 0.0f),
                         -par.integralMaxMm, par.integralMaxMm);
      alvoProp = par.kp * erro_;
    }
    // Sem parede a parte proporcional volta a zero pela taxa e a integral fica congelada.
    proporcional = aproximar(proporcional, alvoProp, par.taxaMmPorS * dt);
    alvo_ = limitar(-(proporcional + integral), -par.alvoMaxMm, par.alvoMaxMm);
    return alvo_;
  }

  ModoCentro modo() const { return modo_; }
  float erro() const { return erro_; }
  float alvo() const { return alvo_; }
  float referencia() const { return refAprendida >= 0 ? refAprendida : par.refFixaMm; }

private:
  void escolherModo(LateralLida e, LateralLida d) {
    const float meiaAssim = 0.5f * par.assimetriaMm;
    erro_ = NAN;
    if (e.valida && d.valida) {
      modo_ = ModoCentro::DUAS;
      erro_ = 0.5f * ((e.mm - d.mm) - par.assimetriaMm);
      const float ref = 0.5f * (e.mm + d.mm);
      refAprendida = refAprendida < 0 ? ref : refAprendida + par.aprendizado * (ref - refAprendida);
      return;
    }
    const float ref = referencia();
    if (ref >= 0 && e.valida) {
      modo_ = ModoCentro::ESQUERDA;
      erro_ = e.mm - (ref + meiaAssim);
    } else if (ref >= 0 && d.valida) {
      modo_ = ModoCentro::DIREITA;
      erro_ = (ref - meiaAssim) - d.mm;
    } else {
      modo_ = ModoCentro::NENHUMA;
    }
  }

  static float limitar(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
  static float aproximar(float atual, float alvo, float passo) {
    return atual + limitar(alvo - atual, -passo, passo);
  }

  ParametrosCentro par;
  ModoCentro modo_ = ModoCentro::NENHUMA;
  float erro_ = NAN;
  float integral = 0, proporcional = 0, alvo_ = 0;
  float refAprendida = -1;
};
