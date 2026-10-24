#include "imu.h"
#include "config.h"
#include "imu_chip_id.h"
#include <Wire.h>
#include <math.h>

namespace {
DadosIMU d;
uint8_t endereco = 0;
uint32_t ultimoUs = 0;
uint32_t falhasSeguidas = 0;
double soma = 0, somaQuadrados = 0;

bool escrever(uint8_t reg, uint8_t valor) {
    Wire.beginTransmission(endereco);
    Wire.write(reg); Wire.write(valor);
    return Wire.endTransmission() == 0;
}
bool lerGiroZ(int16_t &valor) {
    Wire.beginTransmission(endereco);
    Wire.write(0x47);  // GYRO_ZOUT_H
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)endereco, 2) != 2) return false;
    const uint16_t alto = (uint8_t)Wire.read();
    const uint16_t baixo = (uint8_t)Wire.read();
    valor = (int16_t)((alto << 8) | baixo);
    return true;
}
}
void iniciarIMU() {
    d = DadosIMU{};
    endereco = 0;
    falhasSeguidas = 0;
    const uint8_t enderecos[2] = {0x68, 0x69};
    bool viuEndereco = false, identificacaoErrada = false;
    // Algumas placas demoram a estabilizar apos ligar os reguladores.
    // Tres tentativas com pequeno intervalo; sem mascarar falhas eletricas.
    for (int tentativa = 0; tentativa < 3 && !endereco; ++tentativa) {
        for (uint8_t addr : enderecos) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() != 0) continue;
            viuEndereco = true;
            Wire.beginTransmission(addr);
            Wire.write((uint8_t)0x75); // WHO_AM_I
            if (Wire.endTransmission(false) != 0 || Wire.requestFrom((int)addr, 1) != 1) {
                continue;
            }
            const uint8_t identidade = (uint8_t)Wire.read();
            d.enderecoI2C = addr;
            d.whoAmI = identidade;
            Serial.printf("MPU candidato 0x%02X responde WHO_AM_I=0x%02X\n", addr, identidade);
            if (imuIdentidadeSuportada(identidade)) {
                // Confirma a identificacao em 3 leituras para evitar um
                // falso positivo provocado por uma transacao instavel.
                bool estavel = true;
                for (int confirmacao = 0; confirmacao < 2; ++confirmacao) {
                    Wire.beginTransmission(addr);
                    Wire.write((uint8_t)0x75);
                    if (Wire.endTransmission(false) != 0 ||
                        Wire.requestFrom((int)addr, 1) != 1 ||
                        (uint8_t)Wire.read() != identidade) {
                        estavel = false;
                        break;
                    }
                }
                if (estavel) {
                    endereco = addr;
                    Serial.printf("IMU identificada: %s (WHO=0x%02X, I2C=0x%02X)\n",
                                  imuModelo(identidade), identidade, endereco);
                    break;
                }
                Serial.println("IMU: WHO_AM_I instavel, repetir diagnostico I2C.");
            }
            identificacaoErrada = true;
        }
        if (!endereco) delay(120);
    }
    if (!endereco) {
        d.falhaInit = !viuEndereco ? 1 : (identificacaoErrada ? 3 : 2);
        Serial.printf("MPU AUSENTE/INVALIDO: diagnostico=%u. Consulte /diagnostico-i2c.\n", d.falhaInit);
        return;
    }
    d.falhaInit = 0;
    d.presente = escrever(0x6B, 0x01) && // acordar MPU e PLL eixo X
                 escrever(0x1B, 0x00) && // +-250 graus/segundo
                 escrever(0x1A, 0x03) && // filtro DLPF
                 escrever(0x19, 0x04);   // 200 Hz com DLPF ligado
    if (!d.presente) {
        d.falhaInit = 4;
        Serial.println("MPU respondeu ao WHO_AM_I mas falhou ao configurar registradores.");
        return;
    }
    iniciarCalibracaoIMU();
    Serial.printf("IMU %s pronta: endereco=0x%02X WHO=0x%02X. Calibrando parada.\n", imuModelo(d.whoAmI), endereco, d.whoAmI);
}
bool iniciarCalibracaoIMU() {
    if (!d.presente) return false;
    d.calibrada = false; d.confiavel = false;
    d.calibrando = true; d.amostrasBias = 0;
    d.yawDireitaGraus = 0; d.taxaDireitaDps = 0;
    soma = somaQuadrados = 0;
    falhasSeguidas = 0;
    ultimoUs = micros();
    return true;
}
void atualizarIMU() {
    if (!d.presente) return;
    const uint32_t agora = micros();
    const uint32_t intervalo = agora - ultimoUs;
    if (intervalo < IMU_INTERVALO_US) return;
    ultimoUs = agora;
    // Pulando intervalos longos: nao integrar uma amostra antiga como nova.
    if (intervalo > 100000UL) {
        d.confiavel = false;
    }
    int16_t gz;
    if (!lerGiroZ(gz)) {
        ++d.falhasI2C; ++falhasSeguidas;
        if (falhasSeguidas >= 3) d.confiavel = false;
        return;
    }
    falhasSeguidas = 0;
    // MPU-6050 e MPU-6515 usam 131 LSB/(graus/s) para FS=+-250dps.
    // Validar essa escala com um giro MANUAL de 90 graus antes de controle IMU.
    const float taxaRaw = (float)gz / 131.0f;
    d.ultimaLeituraMs = millis();
    if (d.calibrando) {
        soma += taxaRaw;
        somaQuadrados += (double)taxaRaw * taxaRaw;
        ++d.amostrasBias;
        if (d.amostrasBias >= IMU_AMOSTRAS_BIAS) {
            const double n = (double)d.amostrasBias;
            const double media = soma / n;
            const double variancia = fmax(0.0, somaQuadrados / n - media * media);
            d.biasDps = (float)media;
            d.ruidoDps = (float)sqrt(variancia);
            d.calibrando = false;
            d.calibrada = d.ruidoDps <= IMU_RUIDO_MAX_DPS;
            d.confiavel = d.calibrada;
            if (!d.calibrada) Serial.println("IMU: calibracao recusada, vibracao/movimento detectado.");
            else Serial.printf("IMU: bias=%.3f dps; ruido=%.3f dps.\n", d.biasDps, d.ruidoDps);
        }
        return;
    }
    d.taxaDireitaDps = IMU_SINAL_DIREITA * (taxaRaw - d.biasDps);
    if (d.calibrada && d.confiavel && intervalo <= 100000UL) {
        d.yawDireitaGraus += d.taxaDireitaDps * (intervalo * 1e-6f);
    }
}
bool zerarYawIMU() {
    if (!d.presente || !d.calibrada || !d.confiavel ||
        millis() - d.ultimaLeituraMs > IMU_MAX_IDADE_MS ||
        fabsf(d.taxaDireitaDps) > IMU_TAXA_PARADO_MAX_DPS) return false;
    d.yawDireitaGraus = 0;
    ultimoUs = micros();
    return true;
}
DadosIMU lerIMU() {
    DadosIMU resposta = d;
    resposta.confiavel = d.confiavel && d.calibrada &&
        (millis() - d.ultimaLeituraMs <= IMU_MAX_IDADE_MS);
    return resposta;
}
