#pragma once

#include <Arduino.h>

#include <map>
#include <utility>

class GridMap {
 public:
  enum Cell : uint8_t { DESCONHECIDO = 0, LIVRE = 1, OBSTACULO = 2 };

  static const int ALCANCE_AVALIACAO = 3;
  static const int MAX_PATH = 1024;

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
  void markRay(int direction, float distanceCm);
  int unknownAhead(int direction) const;
  int visitedAhead(int direction) const;

  Cell cellAt(int gx, int gy) const;
  Cell cellInDirection(int direction) const;
  int pathLength() const;
  int pathPointX(int index) const;
  int pathPointY(int index) const;

  const std::map<std::pair<int, int>, Cell>& allCells() const { return cells; }

  int visitedCount() const;
  int obstacleCount() const;
  int unknownCount() const;
  void printMap() const;
  static char headingChar(int direction);

 private:
  int wx = 0;
  int wy = 0;
  int dir = 0;

  std::map<std::pair<int, int>, Cell> cells;

  int pathX[MAX_PATH];
  int pathY[MAX_PATH];
  int pathLen = 0;

  void appendPath();
  void bounds(int& minX, int& minY, int& maxX, int& maxY) const;
  static void dxdy(int direction, int& dx, int& dy);
};
