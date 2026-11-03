#include "WebMap.h"

#include <WiFi.h>

#include "Config.h"
#include "GridMap.h"

namespace {

const char* PAGE_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Megatron - Trajetoria</title>
<style>
  body { font-family: sans-serif; background: #1e1e1e; color: #eee; margin: 0; display: flex; flex-direction: column; align-items: center; padding: 12px; }
  h1 { margin: 8px 0 4px; font-size: 1.2rem; }
  #info { font-size: 0.85rem; color: #bbb; margin-bottom: 6px; }
  .controls { margin-bottom: 8px; }
  button { background: #3498db; color: #fff; border: none; padding: 8px 14px; border-radius: 6px; cursor: pointer; font-size: 0.9rem; }
  button:hover { background: #2980b9; }
  canvas { background: #0d0d0d; border: 1px solid #444; max-width: 100%; }
  .legend { display: flex; gap: 16px; margin-top: 8px; font-size: 0.8rem; flex-wrap: wrap; justify-content: center; }
  .legend span { display: inline-flex; align-items: center; gap: 6px; }
  .dot { width: 12px; height: 12px; display: inline-block; border-radius: 2px; }
</style>
</head>
<body>
<h1>Megatron - Trajetoria e Obstaculos</h1>
<div id="info">Conectando...</div>
<div class="controls"><button id="toggle">Visao: seguir o robo</button></div>
<canvas id="map" width="630" height="630"></canvas>
<div class="legend">
  <span><span class="dot" style="background:#2ecc71"></span> Trajetoria (livre)</span>
  <span><span class="dot" style="background:#f1c40f"></span> Caminho percorrido</span>
  <span><span class="dot" style="background:#e74c3c"></span> Obstaculo</span>
  <span><span class="dot" style="background:#1a1a1a"></span> Desconhecido</span>
  <span><span class="dot" style="background:#3498db"></span> Robo</span>
</div>
<script>
const canvas = document.getElementById('map');
const ctx = canvas.getContext('2d');
const info = document.getElementById('info');
const toggle = document.getElementById('toggle');
const headingDirs = [[0,-1],[1,0],[0,1],[-1,0]];
const headingNames = ['N','L','S','O'];
const WINDOW = 21;
let mode = 'follow';

toggle.addEventListener('click', () => {
  mode = (mode === 'follow') ? 'full' : 'follow';
  toggle.textContent = 'Visao: ' + (mode === 'follow' ? 'seguir o robo' : 'ver tudo');
});

async function poll() {
  try {
    const res = await fetch('/state');
    const s = await res.json();
    draw(s);
  } catch (e) {
    info.textContent = 'Sem conexao com o robo';
  }
}

function draw(s) {
  const rx = s.robot[0];
  const ry = s.robot[1];
  const heading = s.robot[2];

  ctx.clearRect(0, 0, canvas.width, canvas.height);

  let scale, offX, offY;
  if (mode === 'follow') {
    scale = canvas.width / WINDOW;
    offX = (rx - Math.floor(WINDOW / 2)) * scale;
    offY = (ry - Math.floor(WINDOW / 2)) * scale;
  } else {
    let minX = rx, maxX = rx, minY = ry, maxY = ry;
    s.cells.forEach(c => {
      minX = Math.min(minX, c[0]); maxX = Math.max(maxX, c[0]);
      minY = Math.min(minY, c[1]); maxY = Math.max(maxY, c[1]);
    });
    s.path.forEach(p => {
      minX = Math.min(minX, p[0]); maxX = Math.max(maxX, p[0]);
      minY = Math.min(minY, p[1]); maxY = Math.max(maxY, p[1]);
    });
    minX -= 1; maxX += 1; minY -= 1; maxY += 1;
    const bw = maxX - minX + 1;
    const bh = maxY - minY + 1;
    scale = Math.min(canvas.width / bw, canvas.height / bh);
    offX = minX * scale - (canvas.width - bw * scale) / 2;
    offY = minY * scale - (canvas.height - bh * scale) / 2;
  }

  const toX = wx => wx * scale - offX;
  const toY = wy => wy * scale - offY;

  s.cells.forEach(c => {
    ctx.fillStyle = c[2] === 2 ? '#e74c3c' : '#2ecc71';
    ctx.fillRect(toX(c[0]), toY(c[1]), scale + 0.5, scale + 0.5);
  });

  if (mode === 'follow') {
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 0.5;
    for (let i = 0; i <= WINDOW; i++) {
      ctx.beginPath(); ctx.moveTo(i * scale, 0); ctx.lineTo(i * scale, canvas.height); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(0, i * scale); ctx.lineTo(canvas.width, i * scale); ctx.stroke();
    }
  }

  if (s.path && s.path.length > 1) {
    ctx.strokeStyle = '#f1c40f';
    ctx.lineWidth = 2;
    ctx.beginPath();
    s.path.forEach((p, i) => {
      const px = toX(p[0]) + scale / 2;
      const py = toY(p[1]) + scale / 2;
      if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
    });
    ctx.stroke();
  }

  const cx = toX(rx) + scale / 2;
  const cy = toY(ry) + scale / 2;
  const radius = scale * 0.38;
  ctx.fillStyle = '#3498db';
  ctx.beginPath();
  ctx.arc(cx, cy, radius, 0, Math.PI * 2);
  ctx.fill();

  const d = headingDirs[heading] || [0, -1];
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + d[0] * radius, cy + d[1] * radius);
  ctx.stroke();

  info.textContent = 'Posicao (' + rx + ',' + ry + ') | Direcao ' + headingNames[heading] +
    ' | Visao: ' + (mode === 'follow' ? 'seguir o robo' : 'ver tudo');
}

setInterval(poll, 500);
poll();
</script>
</body>
</html>
)rawliteral";

}  // namespace

WebMap::WebMap() : server(Config::PORTA_WEB) {}

bool WebMap::begin(const GridMap& gridMap) {
  map = &gridMap;

  WiFi.mode(WIFI_AP);
  const bool apOk = WiFi.softAP(Config::WIFI_SSID, Config::WIFI_PASS);

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/state", HTTP_GET, [this]() { handleState(); });
  server.onNotFound([this]() { handleNotFound(); });
  server.begin();

  Serial.println();
  Serial.println("=== PAGINA WEB ===");
  Serial.print("Conecte-se ao WiFi: ");
  Serial.println(Config::WIFI_SSID);
  Serial.print("Acesse em: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("==================");
  Serial.println();

  return apOk;
}

void WebMap::update() {
  server.handleClient();
}

void WebMap::handleRoot() {
  server.send(200, "text/html", PAGE_HTML);
}

void WebMap::handleState() {
  server.send(200, "application/json", stateJson());
}

void WebMap::handleNotFound() {
  server.send(404, "text/plain", "Nao encontrado");
}

String WebMap::stateJson() const {
  if (map == nullptr) {
    return "{}";
  }

  String json = "{\"robot\":[";
  json += map->x();
  json += ',';
  json += map->y();
  json += ',';
  json += map->heading();
  json += ']';

  json += ",\"path\":[";
  const int length = map->pathLength();
  for (int i = 0; i < length; ++i) {
    if (i != 0) {
      json += ',';
    }
    json += '[';
    json += map->pathPointX(i);
    json += ',';
    json += map->pathPointY(i);
    json += ']';
  }
  json += ']';

  json += ",\"cells\":[";
  bool first = true;
  for (const auto& entry : map->allCells()) {
    if (!first) {
      json += ',';
    }
    first = false;
    json += '[';
    json += entry.first.first;
    json += ',';
    json += entry.first.second;
    json += ',';
    json += static_cast<int>(entry.second);
    json += ']';
  }
  json += ']';

  json += '}';
  return json;
}
