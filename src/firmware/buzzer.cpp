#include <Arduino.h>
#include <esp_arduino_version.h>
#include "buzzer.h"
#include "config.h"
#include "battery.h"
#include "navigation.h"
#include "web_telemetry.h"

namespace {
// ESP32 classico: canais 8..15 pertencem a outro grupo LEDC (low speed).
// Motores sao inicializados primeiro e usam os canais automaticos iniciais.
constexpr uint8_t channel=14;
constexpr uint8_t resolution=10;
buzzer::Player player;
bool ready=false, reverseLeft=false, reverseRight=false;
uint16_t previousHz=0;
// Ajuste acustico experimental. Frequencias finais, sem multiplicador global.
// Referencia: ultima nota do boot (1047 Hz x 2 = 2094 Hz).
uint16_t tunedHz(buzzer::Effect effect, uint16_t hz) {
 if(!hz) return 0;
 switch(effect) {
 case buzzer::Effect::Boot:
  // Aceita tambem a variante do boot sugerida anteriormente.
  switch(hz) {
   case 523: case 880: return 1760;
   case 659: case 932: return 1864;
   case 784: case 988: return 1976;
   case 1047: return 2094;
   default: return 2094;
  }
 case buzzer::Effect::Low:
  switch(hz) {
   case 880: return 2094;
   case 659: return 1976;
   case 440: return 1864;
   default: return 2094;
  }
 case buzzer::Effect::Critical:
 case buzzer::Effect::Reverse:
  return 2094;
 case buzzer::Effect::Start:
  switch(hz) {
   case 659: return 1864;
   case 988: return 2094;
   case 1319: return 2349;
   default: return 2094;
  }
 case buzzer::Effect::Victory:
  // Transposicao de aproximadamente 26 semitons: preserva os intervalos.
  return static_cast<uint16_t>((static_cast<uint32_t>(hz)*449u+50u)/100u);
 default: return hz;
 }
}
void writeTone(uint16_t hz) {
 hz=tunedHz(player.active(),hz);
 if(hz==previousHz) return;
 previousHz=hz;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
 if(!hz) { ledcWrite(PIN_BUZZER,0); return; }
 if(!ledcWriteTone(PIN_BUZZER,hz)) ledcWrite(PIN_BUZZER,0);
#else
 if(!hz) { ledcWrite(channel,0); return; }
 if(!ledcWriteTone(channel,hz)) ledcWrite(channel,0);
#endif
}
}
void initBuzzer() {
 pinMode(PIN_BUZZER,OUTPUT); digitalWrite(PIN_BUZZER,LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
 ready=ledcAttachChannel(PIN_BUZZER,1500,resolution,channel);
 if(ready) ledcWrite(PIN_BUZZER,0);
#else
 ready=ledcSetup(channel,1500,resolution)>0;
 if(ready) { ledcAttachPin(PIN_BUZZER,channel); ledcWrite(channel,0); }
#endif
 if(ready) player.startup();
 else Serial.println("[BUZZER] PWM indisponivel; firmware continua sem som");
}
bool buzzerReady() { return ready; }
void buzzerObserveMotor(bool left,bool reverse) {
 if(left) reverseLeft=reverse; else reverseRight=reverse;
}
bool buzzerTest(buzzer::Effect effect) { return ready && player.request(effect); }
void buzzerCancelTest() { player.cancelTest(); }
void updateBuzzer() {
 if(!ready) return;
 const bool mission=navigationRunning();
 writeTone(player.tick(millis(),bateriaBaixa(),bateriaCritica(),mission,
   navigationCompleted(),reverseLeft&&reverseRight,!mission&&!testeMotorAtivo()));
}