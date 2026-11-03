#pragma once

#include <Arduino.h>
#include <WebServer.h>

class GridMap;

class WebMap {
 public:
  WebMap();

  bool begin(const GridMap& map);
  void update();

 private:
  const GridMap* map = nullptr;
  WebServer server;

  void handleRoot();
  void handleState();
  void handleNotFound();
  String stateJson() const;
};
