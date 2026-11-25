#pragma once
#include <stdint.h>
#include <stddef.h>

// Sequenciador puro: sem Arduino, sem espera, sem acesso aos motores.
namespace buzzer {
enum class Effect : uint8_t { None, Boot, Low, Critical, Start, Reverse, Victory };
struct Note { uint16_t hz, ms; };
struct Pattern { const Note* notes; size_t count; };
constexpr Note boot[] = {{523,100},{0,30},{659,100},{0,30},{784,100},{0,30},{1047,220}};
constexpr Note low[] = {{880,150},{0,70},{659,150},{0,70},{440,260}};
constexpr Note critical[] = {{1800,120},{0,100},{1800,120},{0,100},{1800,120}};
constexpr Note start[] = {{659,110},{0,40},{659,110},{0,40},{988,150},{0,40},{1319,220}};
constexpr Note reverse[] = {{1500,150},{0,350}};
constexpr Note reverseTest[] = {{1500,150},{0,350},{1500,150},{0,350},{1500,150}};
// Frequencias convertidas das notas fornecidas pelo usuario; ritmo escolhido
// para o buzzer, pois a imagem nao informa a duracao das notas.
constexpr Note victory[] = {
 {392,180},{0,25},{392,180},{0,25},{440,180},{0,25},{494,180},{0,25},{587,360},{0,60},
 {587,180},{0,25},{523,180},{0,25},{494,180},{0,25},{392,180},{0,25},{523,360},{0,60},
 {494,180},{0,25},{494,180},{0,25},{440,180},{0,25},{440,180},{0,25},{494,360},{0,60},
 {440,180},{0,25},{440,180},{0,25},{392,180},{0,25},{392,500}
};
template<size_t N> constexpr Pattern patternOf(const Note (&a)[N]) { return {a,N}; }
inline Pattern pattern(Effect e, bool test) {
 switch(e) {
 case Effect::Boot: return patternOf(boot);
 case Effect::Low: return patternOf(low);
 case Effect::Critical: return patternOf(critical);
 case Effect::Start: return patternOf(start);
 case Effect::Reverse: return test ? patternOf(reverseTest) : patternOf(reverse);
 case Effect::Victory: return patternOf(victory);
 default: return {nullptr,0};
 }
}
class Player {
 Effect active_ = Effect::None, pending_ = Effect::None, manual_ = Effect::None;
 Effect battery_ = Effect::None;
 bool manualActive_ = false, mission_ = false, done_ = false;
 size_t index_ = 0;
 uint32_t since_ = 0, lastAlarm_ = 0;
 uint16_t hz_ = 0;
 void stop() { active_ = Effect::None; manualActive_ = false; hz_ = 0; }
 void begin(Effect e, bool test, uint32_t now) {
  active_=e; manualActive_=test; index_=0; since_=now; hz_=pattern(e,test).notes[0].hz;
 }
 void advance(uint32_t now) {
  if(active_==Effect::None) return;
  auto p=pattern(active_,manualActive_);
  if(now-since_<p.notes[index_].ms) return;
  if(++index_>=p.count) { stop(); return; }
  since_=now; hz_=p.notes[index_].hz;
 }
public:
 void startup() { pending_=Effect::Boot; }
 bool request(Effect e) {
  if(e==Effect::None || ((active_==Effect::Critical || active_==Effect::Low) && !manualActive_)) return false;
  manual_=e; return true;
 }
 void cancelTest() { manual_=Effect::None; if(manualActive_) stop(); }
 Effect active() const { return active_; }
 bool isTest() const { return manualActive_; }
 uint16_t tick(uint32_t now,bool lowBattery,bool criticalBattery,bool mission,bool done,bool reversing,bool allowTest) {
  if(mission&&!mission_) { pending_=Effect::Start; cancelTest(); }
  if(done&&!done_) { pending_=Effect::Victory; cancelTest(); }
  if(!mission&&mission_&&!done) {
   if(pending_==Effect::Start) pending_=Effect::None;
   if(active_==Effect::Start&&!manualActive_) stop();
  }
  mission_=mission; done_=done;
  if(!allowTest) cancelTest();
  if(active_==Effect::Reverse&&!manualActive_&&!reversing) stop();
  Effect alarm=criticalBattery?Effect::Critical:lowBattery?Effect::Low:Effect::None;
  bool alarmChanged=alarm!=battery_;
  if(alarmChanged && (active_==Effect::Low||active_==Effect::Critical)&&!manualActive_) stop();
  battery_=alarm;
  if(alarm!=Effect::None && (alarmChanged || now-lastAlarm_ >= (alarm==Effect::Critical?5000u:30000u))) {
   if(active_==Effect::Start||active_==Effect::Victory) {
    if(!manualActive_ && pending_==Effect::None) pending_=active_;
   }
   if(pending_==Effect::Boot) pending_=Effect::None;
   manual_=Effect::None;
   begin(alarm,false,now); lastAlarm_=now; return hz_;
  }
  advance(now);
  if(active_!=Effect::None && !manualActive_ && (active_==Effect::Low||active_==Effect::Critical)) return hz_;
  if(manual_!=Effect::None && allowTest) {
   auto e=manual_; manual_=Effect::None; begin(e,true,now); return hz_;
  }
  if(active_==Effect::None || (active_==Effect::Reverse&&!manualActive_)) {
   if(pending_!=Effect::None) { auto e=pending_; pending_=Effect::None; begin(e,false,now); }
   else if(active_==Effect::None&&reversing) begin(Effect::Reverse,false,now);
  }
  return hz_;
 }
};
}
