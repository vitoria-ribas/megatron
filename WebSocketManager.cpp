#include "WebSocketManager.h"

#include <Arduino.h>
#include "Config.h"
#include "Navigation.h"
#include "WebUI.h"
#include "RobotImage.h"

WebSocketManager::WebSocketManager() : server(Config::PORTA_WEB), ws("/ws") {}

WebSocketManager::~WebSocketManager() {
  dnsServer.stop();
  MDNS.end();
  ws.closeAll();
  server.end();
}

bool WebSocketManager::begin(Navigation& navigation) {
  nav = &navigation;

  Serial.println("\n[WIFI] ===============================================");
  Serial.printf("[WIFI] Configurando Access Point: %s\n", Config::WIFI_SSID);

  WiFi.mode(WIFI_AP);
  WiFi.persistent(false);
  WiFi.setSleep(false); // Mantém o rádio Wi-Fi sempre ativo em potência total
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Potência máxima de transmissão RF (estabilidade superior)

  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  const bool apOk = WiFi.softAP(Config::WIFI_SSID, Config::WIFI_PASS, 1, 0, 4);

  if (apOk) {
    Serial.println("[WIFI] Access Point iniciado com sucesso!");
    Serial.print("[WIFI] Conecte-se na rede Wi-Fi: ");
    Serial.println(Config::WIFI_SSID);
    Serial.print("[WIFI] Endereço IP do Robô: ");
    Serial.println(WiFi.softAPIP());
    Serial.printf("[WIFI] Abra no navegador: http://%s/\n", WiFi.softAPIP().toString().c_str());
    Serial.println("[WIFI] Ou pelo domínio mDNS: http://megatron.local/");
  } else {
    Serial.println("[WIFI] ERRO ao iniciar Access Point!");
    return false;
  }
  Serial.println("[WIFI] ===============================================\n");

  // Inicia Servidor DNS para Captive Portal (redireciona qualquer consulta para o IP do robô)
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Inicia mDNS (permite acessar http://megatron.local em dispositivos compatíveis)
  if (MDNS.begin("megatron")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[MDNS] Respondedor mDNS ativo: http://megatron.local");
  }

  // Configura WebSocket
  ws.onEvent([this](AsyncWebSocket* s, AsyncWebSocketClient* c,
                    AwsEventType t, void* a, uint8_t* d, size_t l) {
    this->onWsEvent(s, c, t, a, d, l);
  });
  server.addHandler(&ws);

  // Rota Principal: Serve o Cyberpunk HUD diretamente da memória Flash (PROGMEM) sem consumir heap
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    const String host = request->host();
    if (host.length() > 0 && host != "192.168.4.1" && host != "megatron.local") {
      request->redirect("http://192.168.4.1/");
      return;
    }
    Serial.printf("[HTTP] Enviando HUD completo (%u bytes) para %s\n", (unsigned)PAGE_HTML_LEN, request->client()->remoteIP().toString().c_str());
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html; charset=utf-8", (const uint8_t*)PAGE_HTML, PAGE_HTML_LEN);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html; charset=utf-8", (const uint8_t*)PAGE_HTML, PAGE_HTML_LEN);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });

  // Tratamento de testes de conectividade de rede (Windows NCSI, Android e Apple)
  // Respondendo positivamente evita que o Windows ou Android derrubem a conexao ou abram msftconnecttest.com
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Microsoft Connect Test");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Microsoft NCSI");
  });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(204);
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(204);
  });
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://192.168.4.1/");
  });
  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("http://192.168.4.1/");
  });

  // Rota da Foto do Modelo do Robô (PROGMEM binário com cache)
  server.on("/robot.jpg", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", ROBOT_JPG, ROBOT_JPG_LEN);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
  });

  // Rota de Emergência HTTP (acesso direto redundante / failsafe)
  server.on("/estop", HTTP_ANY, [this](AsyncWebServerRequest* request) {
    if (nav) nav->emergencyStop();
    request->send(200, "application/json", "{\"estop\":true,\"status\":\"EMERGENCY STOP\"}");
  });

  // Rota de Status HTTP (fallback para curl / inspeção rápida)
  server.on("/state", HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!nav) {
      request->send(500, "text/plain", "Navigation offline");
      return;
    }
    JsonDocument doc;
    const GridMap& map = nav->getMap();
    doc["x"] = map.x();
    doc["y"] = map.y();
    doc["dir"] = (map.heading() * 90) % 360;
    doc["status"] = nav->getModeString();
    doc["battery"] = round(nav->getBatteryPct());
    doc["dist_fwd"] = nav->getLastFrontCm();
    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    const String url = request->url();
    if (url.indexOf("connecttest") != -1) {
      request->send(200, "text/plain", "Microsoft Connect Test");
      return;
    }
    if (url.indexOf("ncsi") != -1) {
      request->send(200, "text/plain", "Microsoft NCSI");
      return;
    }
    if (url.indexOf("204") != -1) {
      request->send(204);
      return;
    }
    request->redirect("http://192.168.4.1/");
  });

  server.begin();
  Serial.println("[HTTP] Servidor Web e WebSocket inicializados na porta 80!");
  return true;
}

void WebSocketManager::onWsEvent(AsyncWebSocket* s, AsyncWebSocketClient* client,
                                 AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WS] Novo cliente #%u conectado de %s\n",
                    client->id(), client->remoteIP().toString().c_str());
      client->setCloseClientOnQueueFull(false);
      sendFullState(client);
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("[WS] Cliente #%u desconectado\n", client->id());
      if (ws.count() == 0 && nav != nullptr && nav->getMode() == MODE_MANUAL) {
        // Se desconectar durante controle manual, para o robô imediatamente por segurança
        nav->manualMove("STOP", 0);
      }
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        handleWsMessage(client, (const char*)data, len);
      }
      break;
    }

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void WebSocketManager::handleWsMessage(AsyncWebSocketClient* client, const char* payload, size_t len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  if (err) {
    Serial.printf("[WS] Erro de parse JSON: %s\n", err.c_str());
    return;
  }

  const char* cmd = doc["cmd"] | "";

  // 1. PING / LATÊNCIA
  if (strcmp(cmd, "ping") == 0) {
    client->text("{\"type\":\"pong\"}");
    return;
  }

  // 2. MOVIMENTO MANUAL (D-PAD / WASD)
  if (strcmp(cmd, "move") == 0) {
    const char* dir = doc["dir"] | "STOP";
    int speed = doc["speed"] | Config::VELOCIDADE_FRENTE;
    if (nav) {
      nav->manualMove(dir, speed);
    }
    return;
  }

  // 3. AJUSTE DE VELOCIDADE EM TEMPO REAL
  if (strcmp(cmd, "speed") == 0) {
    int spd = doc["value"] | Config::VELOCIDADE_FRENTE;
    if (nav) {
      nav->setSpeed(spd);
    }
    broadcastTelemetry(false);
    return;
  }

  // 4. MUDANÇA DE MODO OPERACIONAL
  if (strcmp(cmd, "mode") == 0) {
    const char* val = doc["value"] | "manual";
    if (nav) {
      nav->setMode(val);
    }
    broadcastTelemetry(true);
    return;
  }

  // 4. PARADA DE EMERGÊNCIA
  if (strcmp(cmd, "estop") == 0) {
    if (nav) {
      nav->emergencyStop();
    }
    broadcastTelemetry(false);
    return;
  }

  // 5. NAVEGAÇÃO PARA WAYPOINT (CLICK-TO-NAV NO CANVAS)
  if (strcmp(cmd, "goto") == 0) {
    float gx = doc["x"] | 0.0f;
    float gy = doc["y"] | 0.0f;
    if (nav) {
      nav->setWaypoint(gx, gy);
    }
    broadcastTelemetry(false);
    return;
  }

  // 6. RESET DE MAPA E TRAJETÓRIA
  if (strcmp(cmd, "reset") == 0) {
    if (nav) {
      nav->reset();
    }
    broadcastTelemetry(true);
    return;
  }
}

void WebSocketManager::sendFullState(AsyncWebSocketClient* client) {
  if (client == nullptr || nav == nullptr) return;

  JsonDocument doc;
  const GridMap& map = nav->getMap();

  doc["x"] = map.x();
  doc["y"] = map.y();
  doc["dir"] = (map.heading() * 90) % 360;
  doc["status"] = nav->getModeString();
  doc["action"] = nav->getActionName();
  doc["battery"] = round(nav->getBatteryPct());
  doc["voltage"] = round(nav->getBatteryVoltage() * 10.0f) / 10.0f;
  doc["dist_fwd"] = round(nav->getLastFrontCm() * 10.0f) / 10.0f;
  doc["dist_back"] = round(nav->getLastBackCm() * 10.0f) / 10.0f;
  doc["dist_left"] = round(nav->getLastLeftCm() * 10.0f) / 10.0f;
  doc["dist_right"] = round(nav->getLastRightCm() * 10.0f) / 10.0f;
  doc["explored"] = map.visitedCount();
  doc["pitch"] = nav->getPitch();
  doc["roll"] = nav->getRoll();
  doc["yaw"] = nav->getYaw();
  doc["enabled"] = (nav->getMode() != MODE_ESTOP);
  doc["running"] = nav->isRunning();
  doc["speed"] = nav->getSpeed();
  doc["radar_angle"] = nav->getScannerAngle();
  doc["radar_dist"] = round(nav->getLastScannerDist() * 10.0f) / 10.0f;

  // Histórico de trajetória
  JsonArray pathArr = doc["path"].to<JsonArray>();
  int pLen = map.pathLength();
  for (int i = 0; i < pLen; ++i) {
    JsonArray pt = pathArr.add<JsonArray>();
    pt.add(map.pathPointX(i));
    pt.add(map.pathPointY(i));
  }

  // Células SLAM mapeadas
  JsonArray cellsArr = doc["cells"].to<JsonArray>();
  const auto& allCells = map.allCells();
  for (const auto& entry : allCells) {
    JsonArray c = cellsArr.add<JsonArray>();
    c.add(entry.first.first);
    c.add(entry.first.second);
    c.add((int)entry.second);
  }

  String output;
  serializeJson(doc, output);
  client->text(output);
}

void WebSocketManager::broadcastTelemetry(bool includeMap) {
  if (ws.count() == 0 || nav == nullptr) return;
  if (!ws.availableForWriteAll()) return;

  JsonDocument doc;
  const GridMap& map = nav->getMap();

  doc["x"] = map.x();
  doc["y"] = map.y();
  doc["dir"] = (map.heading() * 90) % 360;
  doc["status"] = nav->getModeString();
  doc["action"] = nav->getActionName();
  doc["battery"] = round(nav->getBatteryPct());
  doc["voltage"] = round(nav->getBatteryVoltage() * 10.0f) / 10.0f;
  doc["dist_fwd"] = round(nav->getLastFrontCm() * 10.0f) / 10.0f;
  doc["dist_back"] = round(nav->getLastBackCm() * 10.0f) / 10.0f;
  doc["dist_left"] = round(nav->getLastLeftCm() * 10.0f) / 10.0f;
  doc["dist_right"] = round(nav->getLastRightCm() * 10.0f) / 10.0f;
  doc["explored"] = map.visitedCount();
  doc["pitch"] = nav->getPitch();
  doc["roll"] = nav->getRoll();
  doc["yaw"] = nav->getYaw();
  doc["enabled"] = (nav->getMode() != MODE_ESTOP);
  doc["running"] = nav->isRunning();
  doc["speed"] = nav->getSpeed();
  doc["radar_angle"] = nav->getScannerAngle();
  doc["radar_dist"] = round(nav->getLastScannerDist() * 10.0f) / 10.0f;

  if (includeMap) {
    JsonArray pathArr = doc["path"].to<JsonArray>();
    int pLen = map.pathLength();
    for (int i = 0; i < pLen; ++i) {
      JsonArray pt = pathArr.add<JsonArray>();
      pt.add(map.pathPointX(i));
      pt.add(map.pathPointY(i));
    }

    JsonArray cellsArr = doc["cells"].to<JsonArray>();
    const auto& allCells = map.allCells();
    for (const auto& entry : allCells) {
      JsonArray c = cellsArr.add<JsonArray>();
      c.add(entry.first.first);
      c.add(entry.first.second);
      c.add((int)entry.second);
    }
  }

  String output;
  serializeJson(doc, output);
  ws.textAll(output);
}

void WebSocketManager::update() {
  dnsServer.processNextRequest();
  ws.cleanupClients();

  const unsigned long now = millis();
  if (now - lastTelemetryAt >= 100) { // Telemetria contínua a 10 Hz (100ms)
    lastTelemetryAt = now;

    bool mapChanged = false;
    if (nav) {
      int currentExplored = nav->getMap().visitedCount() + nav->getMap().obstacleCount();
      if (currentExplored != lastExploredCount) {
        lastExploredCount = currentExplored;
        mapChanged = true;
      }
    }

    // Sincroniza grade completa quando há nova célula mapeada ou a cada 800ms
    bool syncMap = mapChanged || (now - lastMapSyncAt >= 800);
    if (syncMap) {
      lastMapSyncAt = now;
    }

    broadcastTelemetry(syncMap);
  }
}
