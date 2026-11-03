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
  #info { font-size: 0.85rem; color: #bbb; margin-bottom: 8px; }
  canvas { background: #0d0d0d; border: 1px solid #444; max-width: 100%; }
  .legend { display: flex; gap: 16px; margin-top: 8px; font-size: 0.8rem; flex-wrap: wrap; justify-content: center; }
  .legend span { display: inline-flex; align-items: center; gap: 6px; }
  .dot { width: 12px; height: 12px; display: inline-block; border-radius: 2px; }
</style>
</head>
<body>
<h1>Megatron - Trajetoria e Obstaculos</h1>
<div id="info">Conectando...</div>
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
const headingDirs = [[0,-1],[1,0],[0,1],[-1,0]];
const headingNames = ['N','L','S','O'];

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
  const size = s.size;
  const cell = canvas.width / size;

  ctx.clearRect(0, 0, canvas.width, canvas.height);

  for (let r = 0; r < size; r++) {
    for (let c = 0; c < size; c++) {
      const ch = s.cells[r * size + c];
      let color = '#1a1a1a';
      if (ch === 'o') color = '#2ecc71';
      else if (ch === '#') color = '#e74c3c';
      ctx.fillStyle = color;
      ctx.fillRect(c * cell, r * cell, cell + 0.5, cell + 0.5);
    }
  }

  ctx.strokeStyle = '#333';
  ctx.lineWidth = 0.5;
  for (let i = 0; i <= size; i++) {
    ctx.beginPath(); ctx.moveTo(i * cell, 0); ctx.lineTo(i * cell, canvas.height); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(0, i * cell); ctx.lineTo(canvas.width, i * cell); ctx.stroke();
  }

  if (s.path && s.path.length > 1) {
    ctx.strokeStyle = '#f1c40f';
    ctx.lineWidth = 2;
    ctx.beginPath();
    s.path.forEach((p, i) => {
      const px = (p[0] + 0.5) * cell;
      const py = (p[1] + 0.5) * cell;
      if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
    });
    ctx.stroke();
  }

  const rx = (s.x + 0.5) * cell;
  const ry = (s.y + 0.5) * cell;
  const radius = cell * 0.38;
  ctx.fillStyle = '#3498db';
  ctx.beginPath();
  ctx.arc(rx, ry, radius, 0, Math.PI * 2);
  ctx.fill();

  const d = headingDirs[s.heading] || [0,-1];
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(rx, ry);
  ctx.lineTo(rx + d[0] * radius, ry + d[1] * radius);
  ctx.stroke();

  info.textContent = 'Posicao (' + s.x + ',' + s.y + ') | Direcao ' + headingNames[s.heading];
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

  String json = "{\"size\":";
  json += GridMap::TAMANHO;
  json += ",\"x\":";
  json += map->x();
  json += ",\"y\":";
  json += map->y();
  json += ",\"heading\":";
  json += map->heading();

  json += ",\"cells\":\"";
  for (int r = 0; r < GridMap::TAMANHO; ++r) {
    for (int c = 0; c < GridMap::TAMANHO; ++c) {
      switch (map->cellAt(c, r)) {
        case GridMap::LIVRE:
          json += 'o';
          break;
        case GridMap::OBSTACULO:
          json += '#';
          break;
        default:
          json += '.';
          break;
      }
    }
  }
  json += '"';

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

  json += '}';
  return json;
}
