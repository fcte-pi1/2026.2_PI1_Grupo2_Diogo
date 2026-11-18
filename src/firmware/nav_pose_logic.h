#pragma once
#include <math.h>

// ============================================================================
// POSICAO LONGITUDINAL DENTRO DA CELULA  (logica pura, sem Arduino)
// ============================================================================
// O firmware ate a V4.9.3 supunha que toda reta comeca e termina no CENTRO da
// celula: mandava NAV_CELULA_MM e tratava qualquer desvio como falha de pose
// (tolerancia de 0,7 mm). Na pratica isso nunca acontece:
//   - a missao comeca com a traseira encostada na parede, nao no centro;
//   - com parede a frente o robo TOCA a parede antes de alcancar o centro,
//     porque o nariz avanca mais que a meia largura do corredor;
//   - a parada guiada pela borda da abertura termina onde a borda manda.
// Toda vez que isso acontecia a missao caia em FAULT ou o erro se acumulava.
//
// A correcao e guardar UMA grandeza: o deslocamento do eixo em relacao ao
// centro da celula atual, no sentido da marcha.
//
//      offset < 0            offset = 0           offset > 0
//   atras do centro      eixo no centro       a frente do centro
//
// Com ela, toda reta vira "ande o que falta", e nenhuma parada precoce
// invalida a pose: ela apenas muda o offset, que a proxima reta desconta.
// ============================================================================

// --- conversao entre leitura frontal e posicao ------------------------------
// U3 esta u3AoEixo a frente do eixo; a parede da celula esta meiaAbertura a
// frente do centro. Logo  leitura = meiaAbertura - u3AoEixo - offset.
inline float navPoseLeituraFrontalEsperadaMm(float offset, float meiaAbertura, float u3AoEixo) {
    return meiaAbertura - u3AoEixo - offset;
}
inline float navPoseOffsetPelaLeituraFrontalMm(float leitura, float meiaAbertura, float u3AoEixo) {
    return meiaAbertura - u3AoEixo - leitura;
}
// Offset no instante em que a traseira toca a parede de tras (partida).
inline float navPoseOffsetTraseiraEncostadaMm(float meiaAbertura, float eixoATraseira) {
    return eixoATraseira - meiaAbertura;
}
// Offset mais avancado que o robo pode ocupar com parede a frente, mantendo a
// folga pedida. Quando o nariz e mais comprido que a meia largura do corredor
// este valor e NEGATIVO: o robo nunca chega ao centro dessa celula, e isso e
// geometria, nao erro de medicao.
inline float navPoseOffsetMaxComParedeMm(float meiaAbertura, float u3AoEixo,
                                         float u3Toca, float folga) {
    return meiaAbertura - u3AoEixo - (u3Toca + folga);
}

// --- metas de deslocamento --------------------------------------------------
// Quanto andar para sair da celula atual (no offset dado) e parar na celula
// seguinte com o offset desejado. E sempre "o que falta", nunca uma celula fixa.
inline float navPoseMetaRetaMm(float offsetAtual, float celulaMm, float offsetAlvo) {
    return celulaMm - offsetAtual + offsetAlvo;
}
// Deslocamento para reposicionar DENTRO da mesma celula (centragem/partida).
inline float navPoseMetaCentragemMm(float offsetAtual, float offsetAlvo) {
    return offsetAlvo - offsetAtual;
}
// Offset depois de um deslocamento que ATRAVESSOU para a celula seguinte.
inline float navPoseOffsetAposAvancoMm(float offsetInicial, float percorrido, float celulaMm) {
    return offsetInicial + percorrido - celulaMm;
}
// Offset depois de um deslocamento que ficou na MESMA celula.
inline float navPoseOffsetAposAjusteMm(float offsetInicial, float percorrido) {
    return offsetInicial + percorrido;
}

// --- decisoes ---------------------------------------------------------------
// Um offset so e aceito se couber dentro da celula com alguma folga: fora
// disso a odometria perdeu a conta e a pose deixa de ser confiavel.
inline bool navPoseOffsetPlausivel(float offset, float meiaAbertura, float margem) {
    return isfinite(offset) && fabsf(offset) <= meiaAbertura + margem;
}
// Durante o avanco, a leitura frontal diz onde o robo vai parar se mantiver a
// meta. Se isso violar a folga, devolve a meta encurtada; senao devolve NAN.
// Trabalha com o que FALTA: restante = leitura - (u3Toca + folga).
inline float navPoseMetaEncurtadaPorParedeMm(float leitura, float progresso, float metaAtual,
                                             float u3Toca, float folga, float minimoRestante) {
    if (!isfinite(leitura) || !isfinite(progresso) || !isfinite(metaAtual)) return NAN;
    if (progresso < 0.0f || metaAtual <= 0.0f) return NAN;
    const float restante = leitura - (u3Toca + folga);
    const float metaNova = progresso + restante;
    if (!(metaNova < metaAtual)) return NAN;          // a parede nao limita nada
    if (metaNova < progresso + minimoRestante) return NAN; // tarde demais: freia por obstaculo
    return metaNova;
}
// Giro no lugar: o eixo deveria estar no centro da abertura lateral, que e o
// centro da celula. Devolve o AVANCO de correcao, ou 0 se ja esta bom.
// Nunca manda avancar alem do limite imposto por uma parede a frente.
// V5.2: NUNCA devolve re. A versao anterior recuava quando o robo parava mais
// perto da parede que a folga de trabalho, e isso era justamente o contrario
// do que o robo precisa: ele gira encostado (a parte que excede passa por
// cima da parede), e cada milimetro longe dela vira erro na celula seguinte.
// Se o eixo passou do centro, o excesso vira desvio LATERAL depois do giro, e
// a centralizacao da reta seguinte corrige sem manobra extra.
inline float navPoseAjusteAntesDoGiroMm(float offset, float offsetMaximo,
                                        float tolerancia, float maximoAjuste) {
    if (!isfinite(offset)) return 0.0f;
    float alvo = 0.0f;
    if (isfinite(offsetMaximo) && alvo > offsetMaximo) alvo = offsetMaximo;
    float d = alvo - offset;
    if (d <= tolerancia) return 0.0f;      // ja esta no alvo ou a frente dele: gira ali
    if (d > maximoAjuste) d = maximoAjuste;
    return d;
}
