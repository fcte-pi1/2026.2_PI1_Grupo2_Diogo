#pragma once
#include <Arduino.h>

// HTML/CSS/JS completamente embarcado: não necessita backend ou internet.
const char PAGINA_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="pt-BR"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Micromouse V4.2 | Bancada de calibração</title>
<style>
:root{color-scheme:dark;--bg:#101826;--box:#1b2739;--bd:#32435a;--text:#eff5ff;--muted:#9fb1c8;--green:#4ce2a7;--blue:#60b7ff;--red:#ff697d}
*{box-sizing:border-box}body{background:var(--bg);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;padding:22px;line-height:1.5}
main{max-width:1090px;margin:auto}h1{font-size:clamp(1.35rem,3vw,2.3rem);margin:.25em 0}h2{font-size:1.16rem;margin:0 0 12px}p{color:var(--muted);margin:8px 0 14px}small{color:var(--muted)}.eyebrow{color:var(--blue);font-size:.8rem;font-weight:800;letter-spacing:.1em;text-transform:uppercase}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}.card{background:var(--box);border:1px solid var(--bd);border-radius:14px;padding:18px;margin:16px 0;box-shadow:0 6px 20px #0003}.grid>.card{margin:0}.top{display:flex;gap:10px;align-items:center;flex-wrap:wrap}.tag{border:1px solid var(--bd);border-radius:999px;padding:4px 11px;background:#223248;font-weight:700}.tag.good{color:var(--green)}.tag.bad{color:var(--red)}.metric{font-size:1.6rem;font-weight:800;color:var(--green);font-variant-numeric:tabular-nums}.metrics{display:grid;grid-template-columns:1fr 1fr;gap:10px}.metrics>div{background:#142033;border-radius:9px;padding:10px}.label{color:var(--muted);font-size:.79rem;display:block}.unit{font-size:.75rem;color:var(--muted)}.row{display:flex;gap:12px;align-items:end;flex-wrap:wrap}.field{flex:1;min-width:115px}label{display:block;font-size:.86rem;color:var(--muted);margin-bottom:5px}input,select{width:100%;background:#101b2b;color:white;border:1px solid #435a75;border-radius:8px;padding:10px;min-height:40px}button{border:0;border-radius:8px;min-height:42px;background:var(--blue);color:#071b2d;font-weight:750;padding:9px 14px;cursor:pointer}button.secondary{background:#334c69;color:white}button.green{background:var(--green)}button.danger{background:var(--red);color:#22040b;font-weight:900;font-size:1rem}.actions{display:flex;flex-wrap:wrap;gap:9px;margin-top:16px}.notice{padding:10px 14px;border-left:4px solid var(--blue);background:#142238;border-radius:6px;color:#d1e0f1}.warn{border-left-color:var(--red)}.output{background:#0b1421;color:#c4eddf;border-radius:8px;padding:11px;white-space:pre-wrap;font:13px ui-monospace,monospace;overflow-wrap:anywhere}canvas{width:100%;background:#101b2b;border:1px solid #344862;border-radius:8px}a{color:var(--blue)}.bar{height:7px;border-radius:10px;background:#0c1420;overflow:hidden;margin-top:10px}.bar>div{height:100%;background:var(--green);width:0}.footer{font-size:.8rem;color:var(--muted);margin:22px 0}@media(max-width:730px){body{padding:12px}.grid{grid-template-columns:1fr}.card{padding:14px}.metric{font-size:1.25rem}}
</style></head><body><main>
<div class="eyebrow">ESP32 • TB6612FNG • Encoders X4</div>
<h1>Bancada de calibração Micromouse</h1>
<div class="top"><span id="online" class="tag bad">Sem conexão</span><span id="modo" class="tag">—</span><span id="energia" class="tag">INA219: —</span></div>
<p>Conecte ao Wi-Fi <strong>Xaropinho-WiFi</strong>. Para o primeiro ensaio, apoie o robô com as rodas livres e tenha acesso à chave da bateria.</p>
<div class="card"><div class="top"><button class="danger" onclick="parar()">■ FREAR / CANCELAR TESTE</button><button class="secondary" onclick="zerar()">Zerar contadores</button><button class="secondary" onclick="baixarCSV()">Baixar telemetria CSV</button></div><p id="mensagem" class="notice">Sem dados ainda.</p></div>
<div class="grid">
<section class="card"><h2>Motor esquerdo</h2><div class="metrics"><div><span class="label">Contagem acumulada</span><span class="metric" id="contE">—</span></div><div><span class="label">Velocidade (CPS)</span><span class="metric" id="cpsE">—</span></div><div><span class="label">RPM estimado</span><span class="metric" id="rpmE">—</span></div><div><span class="label">PWM aplicado</span><span class="metric" id="pwmE">0</span></div></div></section>
<section class="card"><h2>Motor direito</h2><div class="metrics"><div><span class="label">Contagem acumulada</span><span class="metric" id="contD">—</span></div><div><span class="label">Velocidade (CPS)</span><span class="metric" id="cpsD">—</span></div><div><span class="label">RPM estimado</span><span class="metric" id="rpmD">—</span></div><div><span class="label">PWM aplicado</span><span class="metric" id="pwmD">0</span></div></div></section>
</div>
<section class="card"><h2>1. Descobrir contagens por volta (CPR)</h2>
<p>Marque a roda com uma linha fina. Clique <strong>Iniciar</strong>, dê 10 voltas físicas completas manualmente, sempre no mesmo sentido, e clique <strong>Finalizar</strong>. O motor não será acionado.</p>
<div class="row"><div class="field"><label for="ladoManual">Roda medida</label><select id="ladoManual"><option value="esq">Esquerda</option><option value="dir">Direita</option><option value="ambos">As duas</option></select></div><div class="field"><label for="voltas">Voltas físicas completas</label><input id="voltas" type="number" min="1" max="100" step="1" value="10"></div></div>
<div class="actions"><button class="green" onclick="manualIniciar()">Iniciar medição manual</button><button onclick="manualFinalizar()">Finalizar e calcular</button><button class="secondary" onclick="limparMedicoes()">Limpar medições da sessão</button></div>
<div class="output" id="saidaManual">Ainda sem medições.</div>
<p><small>Resultados em contagens/volta da <strong>roda de saída</strong>, em quadratura X4. Repita 5 vezes por roda; confira os dois sentidos e faça a média das medições válidas.</small></p>
</section>
<section class="card"><h2>2. Registrar CPR validado</h2><p>Preencha o valor médio medido para cada roda. Somente depois de salvar teremos RPM e uma meta teórica de giro. Os valores ficam gravados na memória da ESP32.</p>
<div class="row"><div class="field"><label for="cprE">CPR esquerdo (X4)</label><input type="number" id="cprE" min="1" max="100000" step="0.0001" placeholder="A calibrar"></div><div class="field"><label for="cprD">CPR direito (X4)</label><input type="number" id="cprD" min="1" max="100000" step="0.0001" placeholder="A calibrar"></div></div>
<div class="actions"><button onclick="cprCopiarUltimos()" class="secondary">Copiar médias das medições</button><button class="green" onclick="salvarCPR()">Salvar CPR</button></div>
<div class="output" id="referencia">Meta teórica de 90°: só após CPR medido.</div></section>
<section class="card"><h2>3. Teste temporizado de PWM</h2><p>Rodas suspensas. Teste em malha aberta para medir CPS × PWM. A página envia batimentos de segurança; se a conexão falhar, a ESP32 freia automaticamente. PWM máximo nesta versão: <strong>180/255</strong>.</p>
<div class="row"><div class="field"><label>Lado</label><select id="ladoPwm"><option value="esq">Esquerda</option><option value="dir">Direita</option><option value="ambos">Ambos</option></select></div><div class="field"><label>Tempo (ms)</label><input id="tempoPwm" type="number" min="250" max="10000" value="2500"></div></div>
<div class="row"><div class="field"><label>PWM esquerdo</label><input id="pwmTesteE" type="number" min="0" max="180" value="80"></div><div class="field"><label>Sentido esquerdo</label><select id="sentidoE"><option value="frente">Frente</option><option value="tras">Trás</option></select></div><div class="field"><label>PWM direito</label><input id="pwmTesteD" type="number" min="0" max="180" value="80"></div><div class="field"><label>Sentido direito</label><select id="sentidoD"><option value="frente">Frente</option><option value="tras">Trás</option></select></div></div>
<div class="actions"><button class="green" onclick="pwmIniciar()">▶ Iniciar ensaio</button><button class="danger" onclick="parar()">■ FREAR</button></div>
<div class="bar"><div id="barra"></div></div><div class="output" id="saidaTeste">Nenhum ensaio PWM executado.</div></section>
<section class="card"><h2>4. Controle de velocidade por RPM (V3)</h2>
<p>Malha PI independente em cada motor. <strong>RPM positivo = frente</strong>; RPM negativo = trás; zero = roda freada. O ajuste é preliminar: comece com rodas suspensas e só depois compare no solo.</p>
<div class="row"><div class="field"><label for="rpmAlvoE">RPM alvo esquerda (-120 a 120)</label><input id="rpmAlvoE" type="number" min="-120" max="120" value="60" step="5"></div><div class="field"><label for="rpmAlvoD">RPM alvo direita (-120 a 120)</label><input id="rpmAlvoD" type="number" min="-120" max="120" value="60" step="5"></div><div class="field"><label for="tempoRpm">Duração (ms)</label><input id="tempoRpm" type="number" min="500" max="10000" value="2500"></div></div>
<div class="actions"><button class="green" onclick="iniciarRpm()">▶ Testar RPM</button><button class="secondary" onclick="fixarRpm(40,40)">40 / 40 RPM</button><button class="secondary" onclick="fixarRpm(60,60)">60 / 60 RPM</button><button class="secondary" onclick="fixarRpm(60,0)">60 / 0 RPM</button><button class="secondary" onclick="fixarRpm(0,60)">0 / 60 RPM</button><button class="danger" onclick="parar()">■ FREAR</button></div>
<small>Primeiro teste: 40/0 RPM por 2500 ms; depois 0/40 RPM. Confirme o sentido físico e a contagem positiva antes de testar ambas.</small></section>
<section class="card"><h2>5. Movimento por contagens de encoder (V3)</h2>
<p>Metas relativas ao início do teste. Valores positivos = frente; negativos = trás. Cada motor freia individualmente ao atingir sua meta. O controle <strong>não usa a IMU</strong> e ainda não garante um giro de 90°.</p>
<div class="row"><div class="field"><label for="alvoPosE">Meta esquerda (-2000 a 2000)</label><input id="alvoPosE" type="number" min="-2000" max="2000" step="1" value="100"></div><div class="field"><label for="alvoPosD">Meta direita (-2000 a 2000)</label><input id="alvoPosD" type="number" min="-2000" max="2000" step="1" value="0"></div><div class="field"><label for="velPos">Limite RPM (20 a 75)</label><input id="velPos" type="number" min="20" max="75" value="45"></div><div class="field"><label for="tempoPos">Tempo máximo (ms)</label><input id="tempoPos" type="number" min="500" max="10000" value="5000"></div></div>
<div class="actions"><button class="secondary" onclick="presetPos(100,0)">E +100</button><button class="secondary" onclick="presetPos(0,100)">D +100</button><button class="secondary" onclick="presetPos(300,300)">Ambas +300</button><button class="secondary" onclick="presetPos(600,-600)">Simular giro ±600</button></div>
<div class="actions"><button class="green" onclick="iniciarPosicao()">▶ Executar posição</button><button class="danger" onclick="parar()">■ FREAR</button></div>
<div class="output" id="saidaPosicao">Nenhum ensaio de posição executado.</div>
<small>Inicie apenas com roda esquerda +100, depois direita +100. A opção ±600 exige testes menores aprovados com rodas suspensas; no solo, só em área livre e com verificação externa de ângulo.</small></section>
<section class="card" id="giroV4"><h2>6. Giro comparativo de 90° — V4</h2>
<p>Os dois modos usam <strong>sincronismo entre encoders durante o movimento</strong>. Em ENCODERS, o MPU-6050 é apenas observador. Em IMU, o Yaw relativo determina o final do giro, enquanto os encoders mantêm as rodas sincronizadas. <strong>ENCODERS: primeiro suspenso. IMU: somente no chão, após conferir o sinal do Yaw manualmente.</strong></p>
<div class="metrics"><div><span class="label">IMU MPU-6050 / 6515</span><span class="metric" style="font-size:1rem" id="imuStatus">—</span></div><div><span class="label">Yaw relativo (direita positivo)</span><span class="metric" id="imuYaw">—</span>°</div><div><span class="label">Velocidade angular</span><span class="metric" id="imuTaxa">—</span> °/s</div><div><span class="label">Descompasso rodas</span><span class="metric" id="syncErro">—</span> mm</div></div>
<p class="notice">IMU ausente? <a href="/diagnostico-i2c" target="_blank" rel="noopener">Abrir diagnostico do barramento I2C</a> (GPIO21/22, 100 kHz, WHO_AM_I).</p>
<p id="imuCalib" class="notice">Deixe o robô IMÓVEL ao inicializar para calibrar o bias. Teste a deriva com Yaw zerado por 10 segundos.</p>
<div class="actions"><button class="secondary" onclick="calibrarIMU()">Calibrar MPU parado (~2 s)</button><button class="secondary" onclick="zerarIMU()">Zerar Yaw</button></div>
<div class="row"><div class="field"><label>Algoritmo</label><select id="tipoGiro"><option value="encoder">Encoders (IMU só mede)</option><option value="imu">IMU (alvo angular + sincronismo)</option></select></div><div class="field"><label>Direção do giro</label><select id="ladoGiro"><option value="direita">Direita (+90°)</option><option value="esquerda">Esquerda (−90°)</option></select></div><div class="field"><label>RPM máximo</label><input id="rpmGiro" type="number" min="20" max="60" value="35"></div><div class="field"><label>Timeout (ms)</label><input id="tempoGiro" type="number" min="1000" max="10000" value="6000"></div></div>
<div class="actions"><button class="green" onclick="iniciarGiro()">Iniciar giro 90°</button><button class="danger" onclick="parar()">FREAR</button></div>
<div class="output" id="saidaGiro">Nenhum giro ainda.</div>
<p>Registre <strong>ângulo externo após parar</strong>, 1 s depois da frenagem. A IMU não é uma referência absoluta, e encoders não medem deslizamento.</p>
<canvas id="graficoGiro" width="920" height="160" aria-label="Gráfico de Yaw relativo ao longo do teste"></canvas>
</section>

<section class="card"><h2>6. Frenagem medida na ESP32 (V3)</h2><p>O contador é registrado imediatamente antes do comando de freio, depois aproximadamente 300 ms e 1000 ms. Espere concluir a medição antes de iniciar outro ensaio.</p><div class="output" id="saidaFreio">Nenhuma frenagem capturada.</div></section>
<section class="card"><h2>7. Gráficos de referência × medição</h2><small>Linhas verde/azul = medição E/D; linhas tracejadas = referências E/D. A coleta ocorre enquanto esta página está aberta.</small><p>RPM: alvo e velocidade medida</p><canvas id="graficoRPM" width="800" height="235"></canvas><p>Posição: meta relativa e progresso (contagens)</p><canvas id="graficoPosicao" width="800" height="235"></canvas></section>
<section class="card"><h2>8. Velocidade em tempo real</h2><small>Verde = roda esquerda; azul = roda direita (CPS). Histórico recente sem bibliotecas externas.</small><canvas id="grafico" width="800" height="235"></canvas><div class="top"><span class="tag" id="batV">Tensão: —</span><span class="tag" id="batI">Corrente: —</span></div></section>
<section class="card"><h2>Orientações de segurança</h2><p>O botão vermelho freia por software. Ele não substitui uma chave física que corte a energia dos motores. No chão, faça apenas ensaios supervisionados em área livre, começando por baixas velocidades. Não deixe o robô sem supervisão.</p><div class="notice warn">Se a página parar de enviar sinais por mais de 1,2 s durante um teste PWM, os motores serão frenados. Um ensaio também tem duração máxima de 10 s.</div></section>
<div class="footer">Contagens absolutas de encoder são independentes de CPR. A meta de 90° é geométrica; testes ±600 devem ser validados inicialmente com rodas suspensas e não garantem ângulo exato.</div>
</main><script>
const $=id=>document.getElementById(id);
let ultimo=null,travado=false,amostras=[],historico=[],historicoRPM=[],historicoPosicao=[],historicoGiro=[];
let ultimoStatusOk=0,consultaEmCurso=false;
const limiteHistorico=150;
function fmt(v,n=1){return v===null||v===undefined||!Number.isFinite(Number(v))?'—':Number(v).toFixed(n)}
function msg(t,erro=false){$('mensagem').textContent=t;$('mensagem').className='notice'+(erro?' warn':'')}
async function post(url,dados={}){
  try{
    const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(dados),cache:'no-store'});
    const x=await r.json();if(!r.ok)throw new Error(x.erro||('HTTP '+r.status));
    msg(x.mensagem||'OK');await atualizar();return true;
  }catch(e){msg('Falha: '+e.message,true);return false}
}
function parar(){return post('/api/pwm/stop')}
function zerar(){if(!confirm('As rodas estão completamente paradas?'))return;post('/api/encoders/reset')}
function manualIniciar(){return post('/api/manual/start',{lado:$('ladoManual').value})}
function manualFinalizar(){return post('/api/manual/finish',{voltas:$('voltas').value})}
function limparMedicoes(){if(confirm("Limpar a média e o histórico da sessão? Isso não apaga o CPR salvo."))post("/api/manual/clear")}
function salvarCPR(){return post('/api/cpr',{esq:$('cprE').value,dir:$('cprD').value})}
function pwmIniciar(){if(!confirm('Rodas suspensas, alimentação conferida e chave física acessível?'))return;return post('/api/pwm/start',{lado:$('ladoPwm').value,pwmE:$('pwmTesteE').value,pwmD:$('pwmTesteD').value,duracao:$('tempoPwm').value,sentidoE:$('sentidoE').value,sentidoD:$('sentidoD').value})}
function fixarRpm(e,d){$('rpmAlvoE').value=e;$('rpmAlvoD').value=d}
function presetPos(e,d){$('alvoPosE').value=e;$('alvoPosD').value=d}
function iniciarRpm(){if(!confirm('Teste de RPM: valide PRIMEIRO suspenso; no chão use área livre, supervise e mantenha corte físico acessível.'))return;return post('/api/rpm/start',{rpmE:$('rpmAlvoE').value,rpmD:$('rpmAlvoD').value,duracao:$('tempoRpm').value})}
function calibrarIMU(){if(!confirm('Robo totalmente IMOVEL por pelo menos 2,5 segundos?'))return;return post('/api/imu/calibrar')}
function zerarIMU(){return post('/api/imu/zero')}
function iniciarGiro(){
 const tipo=$('tipoGiro').value;
 if(!confirm(tipo==='imu' ? 'GIRO IMU: chassi DEVE estar no chão (não funciona suspenso), eixo Z vertical, yaw positivo à direita já conferido, área livre e chave física acessível. Continuar?' : 'GIRO ENCODERS: primeiro teste com RODAS SUSPENSAS; no chão só depois, em área livre e com chave física acessível. Continuar?'))return;
 return post('/api/giro/start',{tipo:tipo,lado:$('ladoGiro').value,rpm:$('rpmGiro').value,duracao:$('tempoGiro').value});
}
function iniciarPosicao(){if(!confirm('Teste de POSIÇÃO: confirmou lados e sinais? Primeiro com rodas suspensas; no chão, apenas em área livre e com parada física acessível.'))return;return post('/api/pos/start',{alvoE:$('alvoPosE').value,alvoD:$('alvoPosD').value,rpm:$('velPos').value,duracao:$('tempoPos').value})}
function desenharV3(id,lista,keys){
 const cv=$(id),ctx=cv.getContext('2d'),w=cv.width,h=cv.height;
 ctx.fillStyle='#101b2b';ctx.fillRect(0,0,w,h);
 ctx.strokeStyle='#31435b';ctx.lineWidth=1;
 for(let i=1;i<=4;i++){const y=12+i*(h-28)/5;ctx.beginPath();ctx.moveTo(45,y);ctx.lineTo(w-12,y);ctx.stroke()}
 if(lista.length<2)return;
 const max=Math.max(10,...lista.flatMap(a=>keys.map(k=>Math.abs(Number(a[k.key])||0))))*1.15;
 ctx.font='12px system-ui';ctx.fillStyle='#a4bdd4';ctx.fillText('+'+max.toFixed(0),2,18);ctx.fillText('0',21,h/2);ctx.fillText('-'+max.toFixed(0),2,h-8);
 keys.forEach(({key,color,dash})=>{
  ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=dash?1.5:2.5;ctx.setLineDash(dash?[7,4]:[]);
  lista.forEach((a,i)=>{const x=47+i*(w-64)/(limiteHistorico-1),y=h/2-(Number(a[key])||0)/max*(h-30)/2;if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y)});ctx.stroke();ctx.setLineDash([]);
 });
}
function cprCopiarUltimos(){if(!ultimo)return;if(ultimo.mediaE>0)$('cprE').value=ultimo.mediaE;if(ultimo.mediaD>0)$('cprD').value=ultimo.mediaD}
function desenhar(){
  let cv=$('grafico'),ctx=cv.getContext('2d'),w=cv.width,h=cv.height;
  ctx.clearRect(0,0,w,h);ctx.fillStyle='#101b2b';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#31435b';ctx.lineWidth=1;
  for(let i=1;i<5;i++){let y=20+i*(h-44)/5;ctx.beginPath();ctx.moveTo(40,y);ctx.lineTo(w-12,y);ctx.stroke()}
  if(historico.length<2)return;
  const max=Math.max(10,...historico.map(a=>Math.max(Math.abs(a.e),Math.abs(a.d))))*1.15;
  ctx.font='12px system-ui';ctx.fillStyle='#a4bdd4';ctx.fillText('+'+max.toFixed(0),4,18);ctx.fillText('0',18,h/2);ctx.fillText('-'+max.toFixed(0),3,h-12);
  for(const [key,cor] of [['e','#4ce2a7'],['d','#60b7ff']]){
    ctx.beginPath();ctx.strokeStyle=cor;ctx.lineWidth=2;
    historico.forEach((v,i)=>{const x=43+i*(w-59)/(limiteHistorico-1),y=h/2-v[key]/max*(h-36)/2;if(!i)ctx.moveTo(x,y);else ctx.lineTo(x,y)});ctx.stroke();
  }
}
async function atualizar(){
 if(consultaEmCurso)return;consultaEmCurso=true;
 try{
  const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);
  const d=await r.json();ultimo=d;ultimoStatusOk=Date.now();$('online').textContent='ESP32 conectada';$('online').className='tag good';
  $('modo').textContent='Estado: '+d.modo;$('energia').textContent='INA219: '+(d.inaOk?'OK':'indisponível');
  $('contE').textContent=d.contE;$('contD').textContent=d.contD;
  $('cpsE').textContent=fmt(d.cpsE);$('cpsD').textContent=fmt(d.cpsD);
  $('rpmE').textContent=fmt(d.rpmE);$('rpmD').textContent=fmt(d.rpmD);
  $('pwmE').textContent=d.pwmE;$('pwmD').textContent=d.pwmD;
  $('imuStatus').textContent=d.imuCalibrando?'Calibrando…':(d.imuOk?'Pronta':(d.imuPresente?'Recalibrar':'Ausente'));
  $('imuYaw').textContent=fmt(d.imuYaw,2);
  $('imuTaxa').textContent=fmt(d.imuTaxa,2);
  $('syncErro').textContent=fmt(d.syncErroMm,2);
  $('imuCalib').textContent='Bias Z: '+fmt(d.imuBias,3)+' °/s | Ruído parado: '+fmt(d.imuRuido,3)+' °/s | I2C falhas: '+d.imuFalhasI2C+' | amostras calibração: '+d.imuAmostrasBias+' | orientação: yaw positivo = direita (conferir manualmente).';
  $('saidaGiro').textContent='Tipo: '+d.giroTipo+' | Estado: '+d.modo+' | motivo: '+d.motivo+'\nYaw alvo: '+fmt(d.giroAlvoYaw)+'°; Yaw relativo: '+fmt(d.giroYawRelativo,2)+'°; erro: '+fmt(d.giroErroYaw,2)+'°\nProgresso (mm) E='+fmt(d.syncDistE,2)+' D='+fmt(d.syncDistD,2)+' | Δ='+fmt(d.syncErroMm,2)+' mm | pior Δ='+fmt(d.syncErroMaxMm,2)+' mm\nYaw após freio='+fmt(d.giroYawDepoisFreio,2)+'°; erro='+fmt(d.giroErroDepoisFreio,2)+'°\nCorreção RPM: '+fmt(d.syncCorrecaoRpm,2)+' | Roda pausada E='+d.syncPausaE+' D='+d.syncPausaD+'\nContagens E='+d.progressoE+' / '+d.alvoE+'; D='+d.progressoD+' / '+d.alvoD;
  historicoGiro.push({yaw:Number(d.imuYaw)||0,alvo:d.modo==='GIRO'?Number(d.giroAlvoYaw)||0:0});
  if(historicoGiro.length>limiteHistorico)historicoGiro.shift();
  desenharV3('graficoGiro',historicoGiro,[{key:'yaw',color:'#4ce2a7'},{key:'alvo',color:'#60b7ff',dash:true}]);
  $('batV').textContent='Barramento: '+fmt(d.tensao,2)+' V';$('batI').textContent='INA219: '+fmt(d.corrente,1)+' mA';
  $('saidaManual').textContent=(d.modo==='MANUAL'?'MEDINDO AGORA\n':'ÚLTIMA MEDIÇÃO\n')+'Δ contagens esq: '+(d.modo==='MANUAL'?d.manualDeltaE:d.manualUltDeltaE)+'\nΔ contagens dir: '+(d.modo==='MANUAL'?d.manualDeltaD:d.manualUltDeltaD)+'\nVoltas declaradas: '+d.ultimaQuantidadeVoltas+'\nCPR esq medido: '+fmt(d.ultimaMedidaE,4)+'\nCPR dir medido: '+fmt(d.ultimaMedidaD,4)+'\n\nMédia ESQ ('+d.nE+' medições): '+fmt(d.mediaE,4)+' ± '+fmt(d.desvioE,4)+'\nMédia DIR ('+d.nD+' medições): '+fmt(d.mediaD,4)+' ± '+fmt(d.desvioD,4);
  $('referencia').textContent='CPR salvo: E='+fmt(d.cprE,4)+'; D='+fmt(d.cprD,4)+'\nMeta TEÓRICA por roda no giro de 90°: E='+fmt(d.meta90E,2)+'; D='+fmt(d.meta90D,2)+' contagens';
  $('saidaPosicao').textContent='Progresso E: '+d.progressoE+' / '+d.alvoE+' contagens\nProgresso D: '+d.progressoD+' / '+d.alvoD+' contagens\nErro final/atual E: '+(Number(d.alvoE)-Number(d.progressoE))+' contagens\nErro final/atual D: '+(Number(d.alvoD)-Number(d.progressoD))+' contagens\nRPM solicitada E: '+fmt(d.referenciaRpmE)+'; medida E: '+fmt(d.rpmE)+'\nRPM solicitada D: '+fmt(d.referenciaRpmD)+'; medida D: '+fmt(d.rpmD)+'\nRampa PI E: '+fmt(d.rampaRpmE)+'; D: '+fmt(d.rampaRpmD)+'\nAjuda partida E: '+d.ajudaPartidaE+'; D: '+d.ajudaPartidaD+'\nSem pulsos E: '+d.semPulsosMsE+' ms; D: '+d.semPulsosMsD+' ms\nRoda E concluiu: '+d.chegouE+' | Roda D concluiu: '+d.chegouD;
  $('saidaFreio').textContent=d.freioValido ? 'Contagem no comando: E='+d.freioInicioE+' D='+d.freioInicioD+'\n300 ms: '+(d.freio300Pronto?'E='+d.freio300E+' D='+d.freio300D+' | ΔE='+d.freioDelta300E+' ΔD='+d.freioDelta300D:'ainda não disponível')+'\n1000 ms: '+(d.freio1000Pronto?'E='+d.freio1000E+' D='+d.freio1000D+' | ΔE='+d.freioDelta1000E+' ΔD='+d.freioDelta1000D:'aguardando') : 'Nenhum freio registrado ainda';
  $('saidaTeste').textContent='Estado: '+d.modo+'\nMotivo da última parada: '+d.motivo+'\nTempo restante: '+d.restanteMs+' ms\nΔ último ensaio E: '+d.testeDeltaE+'\nΔ último ensaio D: '+d.testeDeltaD;
  $('barra').style.width=['PWM','RPM','POSICAO','GIRO'].includes(d.modo)&&d.duracaoMs>0?(100*d.decorridoMs/d.duracaoMs).toFixed(1)+'%':'0%';
  if(!$('cprE').dataset.iniciado){if(d.cprE>0)$('cprE').value=d.cprE;if(d.cprD>0)$('cprD').value=d.cprD;$('cprE').dataset.iniciado='1'}
  historico.push({e:Number(d.cpsE)||0,d:Number(d.cpsD)||0});if(historico.length>limiteHistorico)historico.shift();desenhar();
  historicoRPM.push({e:Number(d.rpmE)||0,d:Number(d.rpmD)||0,te:['RPM','POSICAO'].includes(d.modo)?Number(d.referenciaRpmE)||0:0,td:['RPM','POSICAO'].includes(d.modo)?Number(d.referenciaRpmD)||0:0});
  historicoPosicao.push({e:Number(d.progressoE)||0,d:Number(d.progressoD)||0,te:Number(d.alvoE)||0,td:Number(d.alvoD)||0});
  if(historicoRPM.length>limiteHistorico)historicoRPM.shift();if(historicoPosicao.length>limiteHistorico)historicoPosicao.shift();
  desenharV3('graficoRPM',historicoRPM,[{key:'e',color:'#4ce2a7'},{key:'d',color:'#60b7ff'},{key:'te',color:'#4ce2a7',dash:true},{key:'td',color:'#60b7ff',dash:true}]);
  desenharV3('graficoPosicao',historicoPosicao,[{key:'e',color:'#4ce2a7'},{key:'d',color:'#60b7ff'},{key:'te',color:'#4ce2a7',dash:true},{key:'td',color:'#60b7ff',dash:true}]);
  amostras.push({hora:new Date().toISOString(),...d});if(amostras.length>3000)amostras.shift();
 }catch(e){if(Date.now()-ultimoStatusOk>800){$('online').textContent='Sem conexão';$('online').className='tag bad'}}
 finally{consultaEmCurso=false}
}
function baixarCSV(){
 if(!amostras.length){msg('Ainda sem dados para exportar',true);return}
 const chaves=['hora','ms','modo','contE','contD','cpsE','cpsD','rpmE','rpmD','pwmE','pwmD','alvoE','alvoD','progressoE','progressoD','referenciaRpmE','referenciaRpmD','rampaRpmE','rampaRpmD','ajudaPartidaE','ajudaPartidaD','semPulsosMsE','semPulsosMsD','realControleRpmE','realControleRpmD','freioValido','freio300Pronto','freio1000Pronto','freioDelta300E','freioDelta300D','freioDelta1000E','freioDelta1000D','tensao','corrente','motivo','cprE','cprD','giroTipo','giroAlvoYaw','giroYawRelativo','giroErroYaw','imuPresente','imuCalibrada','imuCalibrando','imuOk','imuEndereco','imuWhoAmI','imuFalhaInit','imuYaw','imuTaxa','imuBias','imuRuido','imuFalhasI2C','syncErroMm','syncErroMaxMm','giroYawDepoisFreio','giroErroDepoisFreio','syncDistE','syncDistD','syncCorrecaoRpm','syncPausaE','syncPausaD'];
 const linhas=[chaves.join(';'),...amostras.map(a=>chaves.map(k=>String(a[k]??'').replaceAll(';',',')).join(';'))];
 const blob=new Blob(['\ufeff'+linhas.join('\r\n')],{type:'text/csv;charset=utf-8'});
 const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='micromouse_telemetria_'+Date.now()+'.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
setInterval(atualizar,200);
setInterval(async()=>{if(ultimo&&['PWM','RPM','POSICAO','GIRO'].includes(ultimo.modo)){try{await fetch('/api/heartbeat',{method:'POST',cache:'no-store'})}catch(e){}}},350);
window.addEventListener('keydown',e=>{if(e.key==='Escape')parar()});
atualizar();
</script></body></html>
)HTML";
