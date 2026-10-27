#pragma once

#include <Arduino.h>

class GridMap {
 public:
  enum Cell : uint8_t { DESCONHECIDO = 0, LIVRE = 1, OBSTACULO = 2 };

  static const int TAMANHO = 21;
  static const int ORIGEM = TAMANHO / 2;
  static const int ALCANCE_AVALIACAO = 3;

  void begin();
  void reset();

  int x() const;
  int y() const;
  int heading() const;
  int headingLeft() const;
  int headingRight() const;

  void stepForward();
  void turnLeft();
  void turnRight();

  void markVisited();
  void markObstacle(int direction);
  int unknownAhead(int direction) const;
  int visitsHere() const;
  int visitsAhead(int direction) const;

  int visitedCount() const;
  int obstacleCount() const;
  int unknownCount() const;
  void printMap() const;

 private:
  int posX = ORIGEM;
  int posY = ORIGEM;
  int dir = 0;

  Cell cells[TAMANHO][TAMANHO];
  uint8_t visits[TAMANHO][TAMANHO];

  static void dxdy(int direction, int& dx, int& dy);
  static bool inBounds(int gx, int gy);
  static char headingChar(int direction);
  static char robotChar(int direction);
};
