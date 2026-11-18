#pragma once
#include <math.h>

// ============================================================================
// GEOMETRIA DO ROBO E DO CORREDOR  (logica pura, sem Arduino)
// ============================================================================
// Todo valor aqui vem de REGUA ou de ensaio no labirinto. Nada e chutado.
// As derivacoes abaixo substituem constantes que antes eram estimadas de olho
// (NAV_U2_A_EIXO_MM, NAV_ABERTURA_LARGURA_MM) e que erravam o ponto de giro.
//
// ---------------------------------------------------------------------------
// CONVENCAO LONGITUDINAL (ao longo do sentido de marcha)
//   origem = CENTRO DO EIXO das rodas. E o unico ponto que a odometria mede:
//            o encoder nao sabe onde fica o nariz.
//   +      = sentido do nariz (frente do robo)
// ---------------------------------------------------------------------------
// MEDIDAS DE ENTRADA (as seis que o operador precisa tirar)
//   larguraMm        lateral a lateral do chassi
//   comprimentoMm    traseira a frontal do chassi
//   lateralAoNarizMm recuo da janela dos lasers U2/U4 em relacao ao nariz
//   u3AoNarizMm      recuo da janela do U3 em relacao ao nariz
//   u3TocaMm         leitura REPORTADA pelo U3 com o robo tocando a parede.
//                    NAO e erro de calibracao: e o recuo fisico do sensor
//                    dentro do chassi. Se a parte que toca a parede for mais
//                    avancada que a janela do U3, este valor e maior que zero.
//   bordaAteGiroFrenteMm / bordaAteGiroReMm
//                    ensaio: a partir do instante em que o laser lateral perde
//                    a parede (fim da parede = inicio da abertura), quanto o
//                    robo ainda precisa deslocar ate o eixo ficar no centro da
//                    abertura. Um ensaio andando para frente, outro de re.
// ---------------------------------------------------------------------------
// POR QUE OS DOIS ENSAIOS BASTAM PARA FECHAR A GEOMETRIA
//   Chame s = distancia do laser lateral ao eixo (positiva = laser a FRENTE do
//   eixo) e m = meia largura livre do corredor.
//     andando para frente  o laser cruza a borda e o eixo ainda anda   m + s
//     andando de re        o laser cruza a borda oposta e o eixo anda   m - s
//   Somando e subtraindo:   s = (frente - re)/2     m = (frente + re)/2
//   Os dois ensaios medem s e m de uma vez, ja com o escorregamento real das
//   rodas embutido. E melhor do que medir s com regua e supor m = 168/2.
// ============================================================================

struct GeometriaRobo {
    float larguraMm = 110.0f;
    float comprimentoMm = 120.0f;
    float lateralAoNarizMm = 40.0f;
    float u3AoNarizMm = 60.0f;
    float u3TocaMm = 60.0f;
    float bordaAteGiroFrenteMm = 140.0f;
    float bordaAteGiroReMm = 20.0f;
};

// --- derivadas -------------------------------------------------------------

// s: laser lateral a frente do eixo. Positivo = laser adiante do eixo.
inline float geomLateralAoEixoMm(const GeometriaRobo& g) {
    return 0.5f * (g.bordaAteGiroFrenteMm - g.bordaAteGiroReMm);
}
// m: meia largura livre do corredor, medida pelo proprio robo.
inline float geomMeiaAberturaMm(const GeometriaRobo& g) {
    return 0.5f * (g.bordaAteGiroFrenteMm + g.bordaAteGiroReMm);
}
inline float geomLarguraCorredorMm(const GeometriaRobo& g) {
    return 2.0f * geomMeiaAberturaMm(g);
}
// Quanto o nariz avanca a frente do eixo.
inline float geomEixoAoNarizMm(const GeometriaRobo& g) {
    return g.lateralAoNarizMm + geomLateralAoEixoMm(g);
}
inline float geomEixoATraseiraMm(const GeometriaRobo& g) {
    return g.comprimentoMm - geomEixoAoNarizMm(g);
}
// Quanto a janela do U3 fica a frente do eixo. E o que converte leitura
// frontal em posicao do eixo dentro da celula.
inline float geomU3AoEixoMm(const GeometriaRobo& g) {
    return geomEixoAoNarizMm(g) - g.u3AoNarizMm;
}
// Raio varrido pelo canto mais distante durante um giro no lugar. Usa o maior
// entre nariz e traseira. Serve para saber se o giro cabe no corredor.
inline float geomRaioGiroMm(const GeometriaRobo& g) {
    const float frente = geomEixoAoNarizMm(g);
    const float tras = geomEixoATraseiraMm(g);
    const float maior = frente > tras ? frente : tras;
    const float meia = 0.5f * g.larguraMm;
    return sqrtf(maior * maior + meia * meia);
}
// Giro no lugar so cabe se o raio varrido couber na meia largura do corredor.
// Quando nao cabe, ou a peca que excede esta ACIMA dos 5 cm da parede (passa
// por cima), ou o robo precisa girar numa interseccao.
inline bool geomGiroCabeNoCorredor(const GeometriaRobo& g) {
    return geomRaioGiroMm(g) <= geomMeiaAberturaMm(g);
}
// Coerencia minima antes de deixar a missao partir.
inline bool geomValida(const GeometriaRobo& g) {
    if (!(g.larguraMm > 20.0f && g.larguraMm < 200.0f)) return false;
    if (!(g.comprimentoMm > 20.0f && g.comprimentoMm < 200.0f)) return false;
    if (!(g.lateralAoNarizMm >= 0.0f && g.lateralAoNarizMm <= g.comprimentoMm)) return false;
    if (!(g.u3AoNarizMm >= 0.0f && g.u3AoNarizMm <= g.comprimentoMm)) return false;
    if (!(g.u3TocaMm >= 0.0f && g.u3TocaMm <= g.comprimentoMm)) return false;
    if (!(g.bordaAteGiroFrenteMm > g.bordaAteGiroReMm)) return false;  // s > 0
    const float m = geomMeiaAberturaMm(g);
    if (!(m > 40.0f && m < 120.0f)) return false;
    const float nariz = geomEixoAoNarizMm(g);
    if (!(nariz > 0.0f && nariz < g.comprimentoMm)) return false;
    if (!(geomEixoATraseiraMm(g) > 0.0f)) return false;
    return true;
}
