#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

class Navigation;

class WebSocketManager {
 public:
  WebSocketManager();
  ~WebSocketManager();

  bool begin(Navigation& navigation);
  void update();

  void broadcastTelemetry(bool includeMap = false);

 private:
  Navigation* nav = nullptr;
  AsyncWebServer server;
  AsyncWebSocket ws;
  DNSServer dnsServer;

  unsigned long lastTelemetryAt = 0;
  unsigned long lastMapSyncAt = 0;
  int lastExploredCount = 0;

  void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                 AwsEventType type, void* arg, uint8_t* data, size_t len);
  void handleWsMessage(AsyncWebSocketClient* client, const char* payload, size_t len);
  void sendFullState(AsyncWebSocketClient* client);
};

