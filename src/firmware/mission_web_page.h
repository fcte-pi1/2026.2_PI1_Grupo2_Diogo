#pragma once
#include <Arduino.h>
const char MISSAO_HTML[] PROGMEM = R"MISSAO(<!DOCTYPE html>
<html lang="pt-BR"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Micromouse | Missão</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,sans-serif}body{background:#101724;color:#e7eef8;margin:0;padding:22px}
main{max-width:960px;margin:auto}header{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}
h1{font-size:1.9rem;margin:0}a{color:#7bb9ff}.panel{background:#192437;border:1px solid #324256;padding:18px;border-radius:14px;margin-top:20px}
fieldset{border:0;padding:0;display:flex;flex-wrap:wrap;gap:14px}label{display:flex;flex-direction:column;gap:5px;font-size:.85rem;color:#acbed2}
input,select{width:90px;padding:9px;border:1px solid #586986;border-radius:8px;color:#fff;background:#202e43;font-size:1rem}
button{padding:13px 18px;border-radius:9px;border:0;background:#3984df;color:#fff;cursor:pointer;font-weight:600}button.stop{background:#c9393d}
button:disabled{opacity:.4;cursor:not-allowed}.actions{display:flex;gap:12px;margin-top:14px;flex-wrap:wrap}
pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:.9rem;line-height:1.5}
#grid{display:grid;gap:0;max-width:480px;aspect-ratio:1;margin:20px auto;}
.cell{min-width:0;aspect-ratio:1;position:relative;display:flex;justify-content:center;align-items:center;background:#233349;font-size:.72rem;color:#9eb0c7;}
.cell.current{background:#1d675d;color:#fff;font-weight:700}.cell.goal{background:#694b32}.cell.current.goal{background:#236d57}
.small{font-size:.83rem;color:#bbcee5}.warn{background:#483a20;border-left:4px solid #dea950;padding:12px;border-radius:7px}
span.badge{font-weight:600;color:#93d8c1}
</style></head><body><main>
<header><div><h1>🐭 Micromouse — Missão</h1><p class="small">Mapa interno da ESP32 · navegação independente do Wi-Fi</p></div><a href="/">← Voltar à calibração</a></header>
<div class="panel"><div class="warn">⚠️ V4.9: objetivo padrão no canto diagonal. Ré, centralização e ajuste de abertura exigem validação física; consulte o diagnóstico. O botão PARAR não substitui um corte físico da alimentação dos motores.</div>
<fieldset style="margin-top:17px"><label>Largura (células)<input id="w" type="number" min="2" max="16" value="8"></label>
<label>Altura (células)<input id="h" type="number" min="2" max="16" value="4"></label>
<label>Objetivo X<input id="gx" type="number" min="0" max="15" value="7"></label>
<label>Objetivo Y<input id="gy" type="number" min="0" max="15" value="3"></label>
<label>Início X<input id="sx" type="number" min="0" max="15" value="0"></label>
<label>Início Y<input id="sy" type="number" min="0" max="15" value="0"></label>
<label>Orientação inicial<select id="initialDirection" style="width:auto"><option value="N">Norte ↑</option><option value="L">Leste →</option><option value="S">Sul ↓</option><option value="O">Oeste ←</option></select></label></fieldset>
<p class="small">Coloque o robô no centro da célula de início (X,Y), voltado para a direção escolhida. Origem (0,0) no canto sudoeste; Norte aponta para o lado da altura; Leste para o lado da largura.</p>
<fieldset style="margin-top:14px"><legend>Perfil de navegação por missão (opcional)</legend>
<label><input type="checkbox" id="reverse">Ré em percurso já atravessado (sem sensor traseiro)</label>
<label><input type="checkbox" id="center" checked>Correção lateral (centralizar no corredor)</label>
<label><input type="checkbox" id="opening" checked>Parar guiado pela borda da parede</label>
<label>Largura livre inicial (mm)<input id="widthFree" type="number" placeholder="168" step="0.1"></label>
<label>U2 até eixo (mm)<input id="leftAxle" type="number" value="53" step="0.1"></label>
<label>U4 até eixo (mm)<input id="rightAxle" type="number" value="53" step="0.1"></label>
<label><input type="checkbox" id="supervisor">Geometria, retorno e paradas validados presencialmente pelo responsável</label></fieldset>
<p class="small">Sem sensor traseiro, o retorno depende de trecho previamente percorrido e ambiente estático. Os ajustes são experimentais e limitados; valores de eixo positivos à frente, negativos atrás. Não ligue os perfis sem validação.</p>
<div class="actions"><button id="start" onclick="startMission()">Iniciar exploração</button><button class="stop" onclick="stopMission()">PARAR MOTORES</button><button onclick="refresh()">Atualizar</button></div>
<p id="message" class="small"></p></div>
<section class="panel"><h2>Estado: <span class="badge" id="state">...</span></h2><p id="info" class="small">Carregando ...</p><div id="grid"></div><p class="small">Bordas vermelhas = parede; pontilhadas = desconhecidas; sem borda = passagem confirmada. Coordenadas: origem (0,0) no sudoeste, N aponta para cima.</p></section>
<section class="panel"><h2>Diagnóstico</h2><pre id="status"></pre><p class="small">API do mapa: <a href="/api/maze">/api/maze</a> · Estado da missão: <a href="/api/nav/status">/api/nav/status</a> · Sensores: <a href="/api/lasers/status">/api/lasers/status</a></p></section>
</main><script>
async function req(path,opts){let r=await fetch(path,{cache:'no-store',...(opts||{})});let d=await r.json();if(!r.ok)throw Error(d.erro||JSON.stringify(d));return d}
async function startMission(){let p=new URLSearchParams({largura:document.getElementById('w').value,altura:document.getElementById('h').value,goal_x:document.getElementById('gx').value,goal_y:document.getElementById('gy').value,inicio_x:document.getElementById('sx').value,inicio_y:document.getElementById('sy').value,perfil_v49:'1',direcao_inicial:document.getElementById('initialDirection').value,re_supervisionada:document.getElementById('reverse').checked?'1':'0',centralizar:document.getElementById('center').checked?'1':'0',ajuste_abertura:document.getElementById('opening').checked?'1':'0',validacao_supervisor:document.getElementById('supervisor').checked?'1':'0',largura_livre:document.getElementById('widthFree').value,u2_eixo:document.getElementById('leftAxle').value,u4_eixo:document.getElementById('rightAxle').value});try{await req('/api/nav/start',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p});document.getElementById('message').textContent='Missão iniciada';}catch(e){document.getElementById('message').textContent='Não iniciou: '+e.message}await refresh()}
async function stopMission(){try{await req('/api/nav/stop',{method:'POST'});document.getElementById('message').textContent='Parada solicitada';}catch(e){document.getElementById('message').textContent=e.message}await refresh()}
function corner(){document.getElementById("gx").value=Number(document.getElementById("w").value)-1;document.getElementById("gy").value=Number(document.getElementById("h").value)-1;}
for(const id of ["w","h"])document.getElementById(id).addEventListener("change",corner);
function borders(maskKnown,maskWall){const cols=['top','right','bottom','left'];const colors={};for(let i=0;i<4;i++)colors[cols[i]]=!(maskKnown&(1<<i))?'2px dotted #6a7d93':(maskWall&(1<<i))?'4px solid #e86c66':'2px solid transparent';return colors;}
function draw(m){let el=document.getElementById('grid');el.textContent='';el.style.gridTemplateColumns=`repeat(${m.largura},1fr)`;
 for(let y=m.altura-1;y>=0;y--)for(let x=0;x<m.largura;x++){
  let data=m.celulas[y][x],c=document.createElement('div');c.className='cell'+(m.pose&&m.pose.x===x&&m.pose.y===y?' current':'')+(m.objetivo&&m.objetivo.x===x&&m.objetivo.y===y?' goal':'');
  let cs=borders(data[0],data[1]);for(let [k,v] of Object.entries(cs))c.style['border'+k[0].toUpperCase()+k.slice(1)]=v;
  const arrows={N:'↑',L:'→',S:'↓',O:'←'};c.textContent=(m.pose&&m.pose.x===x&&m.pose.y===y?arrows[m.pose.direcao]:'')+(m.objetivo&&m.objetivo.x===x&&m.objetivo.y===y?'◎':data[2]?'·':'');c.title=`(${x},${y}) paredes=${data[1]} conhecidas=${data[0]}`;el.appendChild(c);
 }}
async function refresh(){try{let [s,m]=await Promise.all([req('/api/nav/status'),req('/api/maze')]);document.getElementById('state').textContent=s.estado;document.getElementById('info').textContent=`Passos: ${s.passos} · Ré: ${s.passosRe||0} · Posição: (${s.x},${s.y}) ${s.direcao}${s.poseConfiavel===false?" (incerta)":""} · Motivo: ${s.motivo||'nenhum'}`;
 document.getElementById('status').textContent=JSON.stringify(s,null,2);document.getElementById('start').disabled=s.ativo;draw(m);
 }catch(e){document.getElementById('status').textContent='Sem conexão: '+e.message}}
refresh();setInterval(refresh,600)
</script></body></html>)MISSAO";
