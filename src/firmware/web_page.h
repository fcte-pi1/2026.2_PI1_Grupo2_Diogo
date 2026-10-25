#pragma once
#include <Arduino.h>

// HTML/CSS/JS completamente embarcado: não necessita backend ou internet.
const char PAGINA_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="pt-BR"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Micromouse V4.5 | Bancada de testes</title>
<style>
:root{color-scheme:dark;--bg:#101826;--box:#1b2739;--bd:#32435a;--text:#eff5ff;--muted:#9fb1c8;--green:#4ce2a7;--blue:#60b7ff;--red:#ff697d}
*{box-sizing:border-box}body{background:var(--bg);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;padding:22px;line-height:1.5}
main{max-width:1090px;margin:auto}h1{font-size:clamp(1.35rem,3vw,2.3rem);margin:.25em 0}h2{font-size:1.16rem;margin:0 0 12px}p{color:var(--muted);margin:8px 0 14px}small{color:var(--muted)}.eyebrow{color:var(--blue);font-size:.8rem;font-weight:800;letter-spacing:.1em;text-transform:uppercase}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}.card{background:var(--box);border:1px solid var(--bd);border-radius:14px;padding:18px;margin:16px 0;box-shadow:0 6px 20px #0003}.grid>.card{margin:0}.top{display:flex;gap:10px;align-items:center;flex-wrap:wrap}.tag{border:1px solid var(--bd);border-radius:999px;padding:4px 11px;background:#223248;font-weight:700}.tag.good{color:var(--green)}.tag.bad{color:var(--red)}.metric{font-size:1.6rem;font-weight:800;color:var(--green);font-variant-numeric:tabular-nums}.metrics{display:grid;grid-template-columns:1fr 1fr;gap:10px}.metrics>div{background:#142033;border-radius:9px;padding:10px}.label{color:var(--muted);font-size:.79rem;display:block}.unit{font-size:.75rem;color:var(--muted)}.row{display:flex;gap:12px;align-items:end;flex-wrap:wrap}.field{flex:1;min-width:115px}label{display:block;font-size:.86rem;color:var(--muted);margin-bottom:5px}input,select{width:100%;background:#101b2b;color:white;border:1px solid #435a75;border-radius:8px;padding:10px;min-height:40px}button{border:0;border-radius:8px;min-height:42px;background:var(--blue);color:#071b2d;font-weight:750;padding:9px 14px;cursor:pointer}button.secondary{background:#334c69;color:white}button.green{background:var(--green)}button.danger{background:var(--red);color:#22040b;font-weight:900;font-size:1rem}.actions{display:flex;flex-wrap:wrap;gap:9px;margin-top:16px}.notice{padding:10px 14px;border-left:4px solid var(--blue);background:#142238;border-radius:6px;color:#d1e0f1}.warn{border-left-color:var(--red)}.output{background:#0b1421;color:#c4eddf;border-radius:8px;padding:11px;white-space:pre-wrap;font:13px ui-monospace,monospace;overflow-wrap:anywhere}canvas{width:100%;background:#101b2b;border:1px solid #344862;border-radius:8px}a{color:var(--blue)}.bar{height:7px;border-radius:10px;background:#0c1420;overflow:hidden;margin-top:10px}.bar>div{height:100%;background:var(--green);width:0}.footer{font-size:.8rem;color:var(--muted);margin:22px 0}@media(max-width:730px){body{padding:12px}.grid{grid-template-columns:1fr}.card{padding:14px}.metric{font-size:1.25rem}}
.abas{display:flex;gap:10px;margin:17px 0;flex-wrap:wrap}.abas button.ativo{background:var(--green);color:#092017}.aba-pagina[hidden]{display:none!important}.sensormini{border-left:4px solid #60b7ff}.legendinha{font-size:.86rem;color:var(--muted)}
</style></head><body><main>
<div class="eyebrow">ESP32 • TB6612FNG • Encoders X4</div>
<h1>Bancada de calibração Micromouse</h1>
<div class="top"><span id="online" class="tag bad">Sem conexão</span><span id="modo" class="tag">—</span><span id="energia" class="tag">INA219: —</span></div>
<p>Conecte ao Wi-Fi <strong>Xaropinho-WiFi</strong>. Para o primeiro ensaio, apoie o robô com as rodas livres e tenha acesso à chave da bateria.</p>
<div class="card"><div class="top"><button class="danger" onclick="parar()">■ FREAR / CANCELAR TESTE</button><button class="secondary" onclick="zerar()">Zerar contadores</button><button class="secondary" onclick="baixarCSV()">Baixar telemetria CSV</button></div><p id="mensagem" class="notice">Sem dados ainda.</p></div>
<nav class="abas" aria-label="Areas de teste"><button id="btAbaCal" class="ativo" onclick="mostrarAba('calibracao')">Motores e giros</button><button id="btAbaLaser" class="secondary" onclick="mostrarAba('sensores')">Navegação · reto e lasers V4.5</button></nav>
<div id="abaCalibracao" class="aba-pagina">
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
<section class="card" id="giroV4"><h2>6. Giro comparativo de 90° — V4.3</h2>
<p>Os dois modos usam <strong>sincronismo entre encoders durante o movimento</strong>. Em ENCODERS, o MPU-6050 é apenas observador. Em IMU, o Yaw relativo determina o final do giro, enquanto os encoders mantêm as rodas sincronizadas. <strong>ENCODERS: primeiro suspenso. IMU: somente no chão, após conferir o sinal do Yaw manualmente.</strong></p>
<div class="metrics"><div><span class="label">IMU MPU-6050 / 6515</span><span class="metric" style="font-size:1rem" id="imuStatus">—</span></div><div><span class="label">Yaw relativo (direita positivo)</span><span class="metric" id="imuYaw">—</span>°</div><div><span class="label">Velocidade angular</span><span class="metric" id="imuTaxa">—</span> °/s</div><div><span class="label">Descompasso rodas</span><span class="metric" id="syncErro">—</span> mm</div></div>
<p class="notice">IMU ausente? <a href="/diagnostico-i2c" target="_blank" rel="noopener">Abrir diagnostico do barramento I2C</a> (GPIO21/22, 100 kHz, WHO_AM_I).</p>
<p id="imuCalib" class="notice">Deixe o robô IMÓVEL ao inicializar para calibrar o bias. Teste a deriva com Yaw zerado por 10 segundos.</p>
<div class="actions"><button class="secondary" onclick="calibrarIMU()">Calibrar MPU parado (~2 s)</button><button class="secondary" onclick="zerarIMU()">Zerar Yaw</button></div>
<p class="hint"><strong>V4.3:</strong> modo <b>Suave</b> evita frear uma roda sozinha durante o giro. O valor X/Y por odometria NAO detecta derrapagem.</p>
<div class="row"><div class="field"><label>Sincronismo</label><select id="syncGiro"><option value="suave" selected>Suave (novo)</option><option value="legado">Antigo (pausa roda)</option></select></div></div>
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

</div>
<div id="abaSensores" class="aba-pagina" hidden>
<section class="card"><h2>V4.4 — Diagnóstico dos sensores VL53L0X</h2>
<p><strong>Somente leitura:</strong> nenhum botão desta aba aciona motores. U2/U3/U4 são os identificadores elétricos; confira a posição física aproximando uma placa de cada janela óptica.</p>
<div class="notice">Largura do chassi <strong>alvo: 110 mm</strong>. Num corredor de 168 mm, folga geométrica nominal ~29 mm de cada lado antes de descontar parafusos, pneus e a posição das janelas ópticas. Leituras inválidas <strong>não significam caminho livre</strong>.</div>
<div class="actions"><button class="secondary" onclick="reinitLasers()">Reiniciar ESP32 e lasers</button><button class="secondary" onclick="baixarLasersLive()">Exportar histórico exibido (CSV)</button><a class="tag" href="/diagnostico-i2c">Scanner I²C</a></div>
<p id="laserAlerta" class="notice">Aguardando primeiras leituras.</p></section>
<section class="card" id="ensaioReto">
<h2>V4.5 — Movimento reto por encoders</h2>
<div class="notice warn"><strong>Primeira gravacao: rodas suspensas.</strong> No chao somente em pista livre, sem paredes proximas, com chave fisica de corte acessivel. <strong>Lasers NAO param o robo e NAO corrigem sua direcao nesta versao.</strong> A IMU apenas mede o Yaw. Mantenha o navegador aberto para o watchdog de 1,2 s.</div>
<p>O alvo e o deslocamento da <strong>media das rodas</strong>. Contagem positiva = frente; negativa = re. A sincronizacao suave diminui a diferenca entre as distancias das rodas durante todo o trajeto. Opcao desligada apenas para comparacao controlada.</p>
<div class="row">
 <div class="field"><label>Distancia assinada (mm)</label><input id="retoDist" type="number" min="-250" max="250" step="5" value="50"></div>
 <div class="field"><label>Limite velocidade (RPM)</label><input id="retoRpm" type="number" min="20" max="60" step="5" value="30"></div>
 <div class="field"><label>Sincronismo</label><select id="retoSync"><option value="suave">Suave (recomendado)</option><option value="desligado">Sem correcao (diagnostico)</option></select></div>
 <div class="field"><label>Timeout (ms)</label><input id="retoTempo" type="number" min="1000" max="10000" step="500" value="7000"></div>
</div>
<div class="actions"><button class="secondary" onclick="presetReto(50,30)">50 mm · 30 RPM</button><button class="secondary" onclick="presetReto(100,30)">100 mm · 30 RPM</button><button class="secondary" onclick="presetReto(150,40)">150 mm · 40 RPM</button><button class="secondary" onclick="presetReto(-50,25)">-50 mm · 25 RPM</button></div>
<div class="actions"><button class="green" onclick="iniciarReto()">▶ Iniciar movimento reto</button><button class="danger" onclick="parar()">■ FREAR AGORA</button></div>
<div class="output" id="saidaReto">Sem ensaio. Use inicialmente as rodas suspensas.</div>
<small>O resultado final de contagens e calculado 1 s apos a frenagem. Um giro de Yaw pequeno nao comprova ausencia de deriva lateral. Compare sempre com uma regua ou marca no piso.</small>
<canvas id="graficoReto" width="800" height="250"></canvas>
<div class="legendinha">Verde = percurso medio; azul tracejado = meta. Apos os testes, exporte o CSV geral pelo botao no topo da pagina.</div>
</section>
<div class="grid">
<section class="card sensormini"><h2>U2 · GPIO 23 · 0x30</h2><label>Posição física (só anotação no navegador)</label><select id="posU2" onchange="guardarPosicoes()"><option value="?">A identificar</option><option value="E">Esquerdo</option><option value="F">Frontal</option><option value="D">Direito</option></select>
<div class="metrics"><div><span class="label">Bruto (mm)</span><span class="metric" id="laserBruto0">—</span></div><div><span class="label">Filtrado (mm)</span><span class="metric" id="laserFiltrado0">—</span></div></div><div class="output" id="laserInfo0">Aguardando</div></section>
<section class="card sensormini"><h2>U3 · GPIO 18 · 0x31</h2><label>Posição física (só anotação no navegador)</label><select id="posU3" onchange="guardarPosicoes()"><option value="?">A identificar</option><option value="E">Esquerdo</option><option value="F">Frontal</option><option value="D">Direito</option></select>
<div class="metrics"><div><span class="label">Bruto (mm)</span><span class="metric" id="laserBruto1">—</span></div><div><span class="label">Filtrado (mm)</span><span class="metric" id="laserFiltrado1">—</span></div></div><div class="output" id="laserInfo1">Aguardando</div></section>
<section class="card sensormini"><h2>U4 · GPIO 19 · 0x32</h2><label>Posição física (só anotação no navegador)</label><select id="posU4" onchange="guardarPosicoes()"><option value="?">A identificar</option><option value="E">Esquerdo</option><option value="F">Frontal</option><option value="D">Direito</option></select>
<div class="metrics"><div><span class="label">Bruto (mm)</span><span class="metric" id="laserBruto2">—</span></div><div><span class="label">Filtrado (mm)</span><span class="metric" id="laserFiltrado2">—</span></div></div><div class="output" id="laserInfo2">Aguardando</div></section>
</div>
<section class="card"><h2>Gráfico em tempo real — leitura bruta × filtrada</h2>
<div class="row"><div class="field"><label>Sensor exibido</label><select id="grafLaserSensor"><option value="0">U2</option><option value="1">U3</option><option value="2">U4</option></select></div></div>
<p class="legendinha">Azul = bruto · Verde = filtrado. Lacunas indicam medidas inválidas. Janela aproximada de 30 segundos, dependendo da rede.</p>
<canvas id="graficoLaser" width="800" height="260"></canvas></section>
<section class="card"><h2>Captura estática de 100 medições</h2>
<p>Coloque uma placa imóvel, meça com régua a distância da <strong>janela óptica</strong> até a placa, escolha o sensor e registre 100 eventos reais diretamente na ESP32. O cálculo ignora leituras inválidas na média, mas informa quantas ocorreram.</p>
<div class="row"><div class="field"><label>Sensor</label><select id="capSensor"><option value="0">U2</option><option value="1">U3</option><option value="2">U4</option></select></div><div class="field"><label>Distância física de referência (mm)</label><input id="capReferencia" type="number" min="20" max="1000" step="1" value="50"></div></div>
<div class="actions"><button class="green" onclick="capturarLaser()">Iniciar 100 medições</button><a id="capBaixar" href="/api/lasers/capture.csv" class="tag" download="vl53l0x_captura.csv">Baixar captura CSV</a><button class="secondary" onclick="limparCaptura()">Limpar captura</button></div>
<div id="capResultado" class="output">Ainda sem captura.</div>
<p><small>Para distâncias de 20–30 mm, um VL53L0X pode retornar dados instáveis. Registre todas as leituras inválidas; não remova falhas do CSV.</small></p></section>
<section class="card"><h2>Roteiro da primeira rodada</h2><p>1) Confirme 0x40, 0x68, 0x30, 0x31 e 0x32 no scanner. 2) Aproxime uma placa, um sensor de cada vez, e identifique U2/U3/U4. 3) Meça 25, 30, 40, 50, 100, 150 e 200 mm; faça 100 amostras por distância e sensor. 4) Aproxime/afaste uma placa para observar o atraso da filtragem. 5) Salve o CSV de cada ensaio. Sem movimento nesta fase.</p>
<div class="notice warn">Não habilitamos ainda a parada frontal nem a centralização pelas paredes. Nenhum controle de movimento usa os resultados dos lasers nesta versão.</div></section>
</div>
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
 return post('/api/giro/start',{tipo:tipo,lado:$('ladoGiro').value,rpm:$('rpmGiro').value,duracao:$('tempoGiro').value,syncGiro:$('syncGiro').value});
}
function presetReto(mm,rpm){$('retoDist').value=mm;$('retoRpm').value=rpm}
function iniciarReto(){
 const dist=Number($('retoDist').value),rpm=Number($('retoRpm').value),t=Number($('retoTempo').value);
 if(!Number.isFinite(dist)||Math.abs(dist)<30||Math.abs(dist)>250||!Number.isFinite(rpm)||rpm<20||rpm>60||!Number.isFinite(t)||t<1000||t>10000){msg('Distancia 30..250 mm; RPM 20..60; timeout 1000..10000 ms',true);return}
 if(!confirm('RETO V4.5: primeiro teste suspenso. No chao, use pista livre e chave fisica de corte. Lasers NAO evitam colisoes. Continuar?'))return;
 return post('/api/reto/start',{distancia:String(dist),rpm:String(rpm),duracao:String(t),sync:$('retoSync').value});
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
  $('saidaGiro').textContent='Tipo: '+d.giroTipo+' | Estado: '+d.modo+' | motivo: '+d.motivo+'\nYaw alvo: '+fmt(d.giroAlvoYaw)+'°; Yaw relativo: '+fmt(d.giroYawRelativo,2)+'°; erro: '+fmt(d.giroErroYaw,2)+'°\nProgresso (mm) E='+fmt(d.syncDistE,2)+' D='+fmt(d.syncDistD,2)+' | Δ='+fmt(d.syncErroMm,2)+' mm | pior Δ='+fmt(d.syncErroMaxMm,2)+' mm\nYaw após freio='+fmt(d.giroYawDepoisFreio,2)+'°; erro='+fmt(d.giroErroDepoisFreio,2)+'°\nSincronismo: '+(d.syncSuave?'SUAVE':'LEGADO')+' | Odometria teórica do eixo X='+fmt(d.odomXmm,2)+' mm, Y='+fmt(d.odomYmm,2)+' mm (NÃO mede derrapagem)'+'\nCorreção RPM: '+fmt(d.syncCorrecaoRpm,2)+' | Roda pausada E='+d.syncPausaE+' D='+d.syncPausaD+'\nContagens E='+d.progressoE+' / '+d.alvoE+'; D='+d.progressoD+' / '+d.alvoD;
  historicoGiro.push({yaw:Number(d.imuYaw)||0,alvo:d.modo==='GIRO'?Number(d.giroAlvoYaw)||0:0});
  if(historicoGiro.length>limiteHistorico)historicoGiro.shift();
  desenharV3('graficoGiro',historicoGiro,[{key:'yaw',color:'#4ce2a7'},{key:'alvo',color:'#60b7ff',dash:true}]);
  const inicioReto=d.retoAlvoMm!==null&&Number(d.retoAlvoMm)!==0;
  if(inicioReto){
   $('saidaReto').textContent='Estado: '+d.modo+' | Motivo: '+d.motivo+'\nAlvo: '+fmt(d.retoAlvoMm,1)+' mm | Media: '+fmt(d.retoProgressoMm,2)+' mm | Erro durante teste: '+fmt(d.retoErroMm,2)+' mm'+
   '\nRoda E: '+fmt(d.retoDistEMm,2)+' mm | Roda D: '+fmt(d.retoDistDMm,2)+' mm'+
   '\nSincronismo: '+(d.retoSyncAtivo?'SUAVE':'SEM CORRECAO')+' | Descompasso: '+fmt(d.retoSyncErroMm,2)+' mm | Pico: '+fmt(d.retoSyncMaxMm,2)+' mm | Correcao: '+fmt(d.retoSyncCorrecaoRpm,2)+' RPM'+
   '\nIMU (somente observa): '+fmt(d.retoYawImu,2)+' graus | Encoder Yaw: '+fmt(d.retoYawEncoder,2)+' graus'+
   '\nOdometria: X='+fmt(d.retoOdomXmm,2)+' mm; Y='+fmt(d.retoOdomYmm,2)+' mm (nao mede escorregamento)'+
   '\nPOS-FREIO (1 s): '+(d.retoFinalPronto?'PRONTO; E='+fmt(d.retoFinalE,2)+' D='+fmt(d.retoFinalD,2)+' media='+fmt(d.retoFinalCentro,2)+' erro='+fmt(d.retoErroFinal,2)+' mm':'aguardando 1 s depois da parada');
  }
  historicoReto.push({alvo:inicioReto?Number(d.retoAlvoMm)||0:0,
                    progresso:inicioReto?Number(d.retoProgressoMm)||0:0});
  if(historicoReto.length>limiteHistorico)historicoReto.shift();
  desenharV3('graficoReto',historicoReto,[{key:'progresso',color:'#4ce2a7'},{key:'alvo',color:'#60b7ff',dash:true}]);
  $('batV').textContent='Barramento: '+fmt(d.tensao,2)+' V';$('batI').textContent='INA219: '+fmt(d.corrente,1)+' mA';
  $('saidaManual').textContent=(d.modo==='MANUAL'?'MEDINDO AGORA\n':'ÚLTIMA MEDIÇÃO\n')+'Δ contagens esq: '+(d.modo==='MANUAL'?d.manualDeltaE:d.manualUltDeltaE)+'\nΔ contagens dir: '+(d.modo==='MANUAL'?d.manualDeltaD:d.manualUltDeltaD)+'\nVoltas declaradas: '+d.ultimaQuantidadeVoltas+'\nCPR esq medido: '+fmt(d.ultimaMedidaE,4)+'\nCPR dir medido: '+fmt(d.ultimaMedidaD,4)+'\n\nMédia ESQ ('+d.nE+' medições): '+fmt(d.mediaE,4)+' ± '+fmt(d.desvioE,4)+'\nMédia DIR ('+d.nD+' medições): '+fmt(d.mediaD,4)+' ± '+fmt(d.desvioD,4);
  $('referencia').textContent='CPR salvo: E='+fmt(d.cprE,4)+'; D='+fmt(d.cprD,4)+'\nMeta TEÓRICA por roda no giro de 90°: E='+fmt(d.meta90E,2)+'; D='+fmt(d.meta90D,2)+' contagens';
  $('saidaPosicao').textContent='Progresso E: '+d.progressoE+' / '+d.alvoE+' contagens\nProgresso D: '+d.progressoD+' / '+d.alvoD+' contagens\nErro final/atual E: '+(Number(d.alvoE)-Number(d.progressoE))+' contagens\nErro final/atual D: '+(Number(d.alvoD)-Number(d.progressoD))+' contagens\nRPM solicitada E: '+fmt(d.referenciaRpmE)+'; medida E: '+fmt(d.rpmE)+'\nRPM solicitada D: '+fmt(d.referenciaRpmD)+'; medida D: '+fmt(d.rpmD)+'\nRampa PI E: '+fmt(d.rampaRpmE)+'; D: '+fmt(d.rampaRpmD)+'\nAjuda partida E: '+d.ajudaPartidaE+'; D: '+d.ajudaPartidaD+'\nSem pulsos E: '+d.semPulsosMsE+' ms; D: '+d.semPulsosMsD+' ms\nRoda E concluiu: '+d.chegouE+' | Roda D concluiu: '+d.chegouD;
  $('saidaFreio').textContent=d.freioValido ? 'Contagem no comando: E='+d.freioInicioE+' D='+d.freioInicioD+'\n300 ms: '+(d.freio300Pronto?'E='+d.freio300E+' D='+d.freio300D+' | ΔE='+d.freioDelta300E+' ΔD='+d.freioDelta300D:'ainda não disponível')+'\n1000 ms: '+(d.freio1000Pronto?'E='+d.freio1000E+' D='+d.freio1000D+' | ΔE='+d.freioDelta1000E+' ΔD='+d.freioDelta1000D:'aguardando') : 'Nenhum freio registrado ainda';
  $('saidaTeste').textContent='Estado: '+d.modo+'\nMotivo da última parada: '+d.motivo+'\nTempo restante: '+d.restanteMs+' ms\nΔ último ensaio E: '+d.testeDeltaE+'\nΔ último ensaio D: '+d.testeDeltaD;
  $('barra').style.width=['PWM','RPM','POSICAO','GIRO','RETO'].includes(d.modo)&&d.duracaoMs>0?(100*d.decorridoMs/d.duracaoMs).toFixed(1)+'%':'0%';
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
 const chaves=['hora','ms','modo','contE','contD','cpsE','cpsD','rpmE','rpmD','pwmE','pwmD','alvoE','alvoD','progressoE','progressoD','referenciaRpmE','referenciaRpmD','rampaRpmE','rampaRpmD','ajudaPartidaE','ajudaPartidaD','semPulsosMsE','semPulsosMsD','realControleRpmE','realControleRpmD','freioValido','freio300Pronto','freio1000Pronto','freioDelta300E','freioDelta300D','freioDelta1000E','freioDelta1000D','tensao','corrente','motivo','cprE','cprD','giroTipo','giroAlvoYaw','giroYawRelativo','giroErroYaw','imuPresente','imuCalibrada','imuCalibrando','imuOk','imuEndereco','imuWhoAmI','imuFalhaInit','imuYaw','imuTaxa','imuBias','imuRuido','imuFalhasI2C','syncErroMm','syncErroMaxMm','syncSuave','odomXmm','odomYmm','odomYawGraus','giroYawDepoisFreio','giroErroDepoisFreio','syncDistE','syncDistD','syncCorrecaoRpm','syncPausaE','syncPausaD','retoAlvoMm','retoProgressoMm','retoErroMm','retoDistEMm','retoDistDMm','retoSyncErroMm','retoSyncMaxMm','retoSyncCorrecaoRpm','retoSyncAtivo','retoYawImu','retoYawEncoder','retoOdomXmm','retoOdomYmm','retoFinalPronto','retoFinalE','retoFinalD','retoFinalCentro','retoErroFinal'];
 const linhas=[chaves.join(';'),...amostras.map(a=>chaves.map(k=>String(a[k]??'').replaceAll(';',',')).join(';'))];
 const blob=new Blob(['\ufeff'+linhas.join('\r\n')],{type:'text/csv;charset=utf-8'});
 const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='micromouse_telemetria_'+Date.now()+'.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}

// ===== V4.4 - Aba de diagnostico dos lasers =====
let historicoReto=[];
let abaAtiva='calibracao', consultaLaserEmCurso=false,ultimoLaser=null;
let historicoLaser=[[],[],[]],historicoLaserExport=[];
function mostrarAba(aba){
  abaAtiva=aba;
  $('abaCalibracao').hidden=aba!=='calibracao';
  $('abaSensores').hidden=aba!=='sensores';
  $('btAbaCal').className=aba==='calibracao'?'ativo':'secondary';
  $('btAbaLaser').className=aba==='sensores'?'ativo':'secondary';
  if(aba==='sensores')atualizarLasersTela();
}
function guardarPosicoes(){
  try{localStorage.setItem('micromousePosLasersV44',JSON.stringify([$('posU2').value,$('posU3').value,$('posU4').value]));}catch(e){}
}
function recuperarPosicoes(){
  try{const v=JSON.parse(localStorage.getItem('micromousePosLasersV44')||'null');if(Array.isArray(v)&&v.length===3){for(let i=0;i<3;i++)$('posU'+(i+2)).value=v[i]}}catch(e){}
}
function fmtLaser(v){return typeof v==='number'&&Number.isFinite(v)?String(v):'—'}
function atualizarCartoesLaser(t){
  const d=t.sensores;
  for(let i=0;i<3;i++){
    const s=d[i],inval=s.status!=='VALIDO'||!s.valido;
    $('laserBruto'+i).textContent=inval?'—':fmtLaser(s.brutoMm);
    $('laserFiltrado'+i).textContent=inval?'—':fmtLaser(s.filtradoMm);
    $('laserInfo'+i).textContent='Sensor: '+(s.inicializado?'ONLINE':'NÃO INICIALIZADO')+' | '+s.status+' | status VL='+s.rangeStatus+
      '\nParede: '+s.parede+' | idade da última leitura válida: '+(s.idadeValidaMs===null?'—':s.idadeValidaMs+' ms')+
      '\nTotal: '+s.total+' | válidas: '+s.validas+' | inválidas: '+s.invalidas+
      '\nPeríodo configurado: '+s.periodoMs+' ms | último intervalo: '+s.intervaloRealMs+' ms';
    const valor=(s.valido&&s.status==='VALIDO')?{raw:s.brutoMm,filt:s.filtradoMm}:{raw:null,filt:null};
    historicoLaser[i].push(valor);if(historicoLaser[i].length>125)historicoLaser[i].shift();
  }
  const cp=t.captura;
  $('capResultado').textContent=cp.ativa?'EM ANDAMENTO — '+cp.amostras+'/'+cp.objetivo+' eventos; sensor U'+(cp.sensor+2):
    (cp.pronta?'CAPTURA '+cp.motivo+' — U'+(cp.sensor+2):'SEM CAPTURA')+
    '\nReferência: '+cp.referenciaMm+' mm | Total: '+cp.amostras+'/'+cp.objetivo+
    ' | Válidas: '+cp.validas+' | Inválidas: '+cp.invalidas+
    '\nMédia bruta: '+fmt(cp.mediaBruta,2)+' mm | Desvio padrão bruto: '+fmt(cp.desvioBruta,2)+' mm'+
    '\nErro médio (bruto - referência): '+(cp.validas?fmt(cp.mediaBruta-cp.referenciaMm,2):'—')+' mm'+
    '\nMédia filtrada: '+fmt(cp.mediaFiltrada,2)+' mm | Duração: '+cp.duracaoMs+' ms';
  const ativos=d.filter(s=>s.inicializado).length;
  $('laserAlerta').textContent='Sensores inicializados: '+ativos+'/3. Endereços esperados: U2=0x30, U3=0x31, U4=0x32. '+
    (ativos<3?'Confira alimentação, XSHUT e I²C; não avance para movimento.':'Leituras prontas para diagnóstico; identifique fisicamente cada posição.');
}
function desenharLaser(){
 const cv=$('graficoLaser');if(!cv)return;
 const ctx=cv.getContext('2d'),w=cv.width,h=cv.height;
 ctx.fillStyle='#101b2b';ctx.fillRect(0,0,w,h);
 const hist=historicoLaser[Number($('grafLaserSensor').value)||0];
 const valid=hist.flatMap(p=>[p.raw,p.filt]).filter(v=>v!==null&&Number.isFinite(v));
 const ymax=valid.length?Math.max(50,Math.ceil(Math.max(...valid)*1.1/10)*10):200;
 ctx.strokeStyle='#31435b';ctx.lineWidth=1;ctx.font='12px system-ui';ctx.fillStyle='#a4bdd4';
 for(let j=0;j<=4;j++){let y=12+j*(h-32)/4;ctx.beginPath();ctx.moveTo(50,y);ctx.lineTo(w-10,y);ctx.stroke();ctx.fillText(String(Math.round(ymax*(4-j)/4)),5,y+4)}
 for(const [key,color] of [['raw','#60b7ff'],['filt','#4ce2a7']]){
  ctx.strokeStyle=color;ctx.lineWidth=2;ctx.beginPath();let aberto=false;
  hist.forEach((p,i)=>{const val=p[key],x=51+i*(w-62)/124;if(val===null){aberto=false;return};const y=h-20-val/ymax*(h-32);if(!aberto){ctx.moveTo(x,y);aberto=true}else ctx.lineTo(x,y)});
  ctx.stroke();
 }
}
async function atualizarLasersTela(){
 if(abaAtiva!=='sensores'||consultaLaserEmCurso)return;
 consultaLaserEmCurso=true;
 try{
   const r=await fetch('/api/lasers/status',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);
   const j=await r.json();ultimoLaser=j;atualizarCartoesLaser(j);desenharLaser();
   const a={tempo:new Date().toISOString(),ms:j.ms};
   for(let i=0;i<3;i++){const d=j.sensores[i];a['u'+(i+2)+'_bruto']=d.brutoMm;a['u'+(i+2)+'_filtrado']=d.filtradoMm;a['u'+(i+2)+'_status']=d.status;a['u'+(i+2)+'_idade_ms']=d.idadeValidaMs}
   historicoLaserExport.push(a);if(historicoLaserExport.length>3000)historicoLaserExport.shift();
 }catch(e){$('laserAlerta').textContent='Falha ao ler sensores: '+e.message}
 finally{consultaLaserEmCurso=false}
}
async function capturarLaser(){
 if(!confirm('Motores PARADOS, placa imóvel e distância da janela óptica medida com régua?'))return;
 return post('/api/lasers/capture/start',{sensor:$('capSensor').value,referencia:$('capReferencia').value,quantidade:'100'});
}
async function reinitLasers(){
 if(!confirm('Reiniciar a ESP32 e os lasers (XSHUT)? Apenas com motores parados.'))return;
 return post('/api/lasers/reinit',{});
}
async function limparCaptura(){return post('/api/lasers/capture/clear',{})}
function baixarLasersLive(){
 if(!historicoLaserExport.length){msg('Nenhuma amostra exibida ainda',true);return}
 const ch=['tempo','ms','u2_bruto','u2_filtrado','u2_status','u2_idade_ms','u3_bruto','u3_filtrado','u3_status','u3_idade_ms','u4_bruto','u4_filtrado','u4_status','u4_idade_ms'];
 const csv=[ch.join(';'),...historicoLaserExport.map(r=>ch.map(k=>String(r[k]??'')).join(';'))].join('\r\n');
 const blob=new Blob(['\ufeff'+csv],{type:'text/csv;charset=utf-8'});
 const url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='micromouse_lasers_visual_'+Date.now()+'.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
}
recuperarPosicoes();
setInterval(atualizarLasersTela,300);

setInterval(atualizar,200);
setInterval(async()=>{if(ultimo&&['PWM','RPM','POSICAO','GIRO','RETO'].includes(ultimo.modo)){try{await fetch('/api/heartbeat',{method:'POST',cache:'no-store'})}catch(e){}}},350);
window.addEventListener('keydown',e=>{if(e.key==='Escape')parar()});
atualizar();
</script></body></html>
)HTML";
