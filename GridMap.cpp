#include "GridMap.h"

#include <Arduino.h>

void GridMap::begin() {
  reset();
}

void GridMap::reset() {
  cells.clear();
  wx = 0;
  wy = 0;
  dir = 0;
  pathLen = 0;
  appendPath();
  markVisited();
}

void GridMap::appendPath() {
  if (pathLen >= MAX_PATH) {
    return;
  }
  pathX[pathLen] = wx;
  pathY[pathLen] = wy;
  ++pathLen;
}

int GridMap::x() const {
  return wx;
}

int GridMap::y() const {
  return wy;
}

int GridMap::heading() const {
  return dir;
}

int GridMap::headingLeft() const {
  return (dir + 3) % 4;
}

int GridMap::headingRight() const {
  return (dir + 1) % 4;
}

void GridMap::dxdy(int direction, int& dx, int& dy) {
  switch (direction) {
    case 0:
      dx = 0;
      dy = -1;
      break;
    case 1:
      dx = 1;
      dy = 0;
      break;
    case 2:
      dx = 0;
      dy = 1;
      break;
    default:
      dx = -1;
      dy = 0;
      break;
  }
}

char GridMap::headingChar(int direction) {
  switch (direction) {
    case 0:
      return 'N';
    case 1:
      return 'L';
    case 2:
      return 'S';
    default:
      return 'O';
  }
}

void GridMap::markVisited() {
  cells[std::make_pair(wx, wy)] = LIVRE;
}

void GridMap::markObstacle(int direction) {
  int dx, dy;
  dxdy(direction, dx, dy);

  const int gx = wx + dx;
  const int gy = wy + dy;

  if (cells.find(std::make_pair(gx, gy)) == cells.end()) {
    cells[std::make_pair(gx, gy)] = OBSTACULO;
  }
}

void GridMap::markRay(int direction, float distanceCm) {
  int dx, dy;
  dxdy(direction, dx, dy);

  if (distanceCm < 0) {
    for (int i = 1; i <= ALCANCE_AVALIACAO; ++i) {
      const int gx = wx + dx * i;
      const int gy = wy + dy * i;
      if (cellAt(gx, gy) == DESCONHECIDO) {
        cells[std::make_pair(gx, gy)] = LIVRE;
      }
    }
    return;
  }

  const int targetCell = (int)roundf(distanceCm / 12.0f);
  const int limit = targetCell < ALCANCE_AVALIACAO ? targetCell : ALCANCE_AVALIACAO;

  for (int i = 1; i < limit; ++i) {
    const int gx = wx + dx * i;
    const int gy = wy + dy * i;
    if (cellAt(gx, gy) == DESCONHECIDO) {
      cells[std::make_pair(gx, gy)] = LIVRE;
    }
  }

  if (limit >= 1 && limit <= ALCANCE_AVALIACAO) {
    const int gx = wx + dx * limit;
    const int gy = wy + dy * limit;
    if (cellAt(gx, gy) != LIVRE) {
      cells[std::make_pair(gx, gy)] = OBSTACULO;
    }
  }
}

void GridMap::stepForward() {
  int dx, dy;
  dxdy(dir, dx, dy);

  wx += dx;
  wy += dy;

  markVisited();
  appendPath();
}

void GridMap::turnLeft() {
  dir = (dir + 3) % 4;
}

void GridMap::turnRight() {
  dir = (dir + 1) % 4;
}

int GridMap::unknownAhead(int direction) const {
  int dx, dy;
  dxdy(direction, dx, dy);

  int count = 0;
  for (int i = 1; i <= ALCANCE_AVALIACAO; ++i) {
    const int gx = wx + dx * i;
    const int gy = wy + dy * i;
    if (cells.find(std::make_pair(gx, gy)) == cells.end()) {
      ++count;
    }
  }
  return count;
}

int GridMap::visitedAhead(int direction) const {
  int dx, dy;
  dxdy(direction, dx, dy);

  int count = 0;
  for (int i = 1; i <= ALCANCE_AVALIACAO; ++i) {
    const int gx = wx + dx * i;
    const int gy = wy + dy * i;
    if (cellAt(gx, gy) == LIVRE) {
      ++count;
    }
  }
  return count;
}

GridMap::Cell GridMap::cellAt(int gx, int gy) const {
  const auto it = cells.find(std::make_pair(gx, gy));
  if (it == cells.end()) {
    return DESCONHECIDO;
  }
  return it->second;
}

GridMap::Cell GridMap::cellInDirection(int direction) const {
  int dx, dy;
  dxdy(direction, dx, dy);
  return cellAt(wx + dx, wy + dy);
}

int GridMap::pathLength() const {
  return pathLen;
}

int GridMap::pathPointX(int index) const {
  if (index < 0 || index >= pathLen) {
    return 0;
  }
  return pathX[index];
}

int GridMap::pathPointY(int index) const {
  if (index < 0 || index >= pathLen) {
    return 0;
  }
  return pathY[index];
}

int GridMap::visitedCount() const {
  int count = 0;
  for (const auto& entry : cells) {
    if (entry.second == LIVRE) {
      ++count;
    }
  }
  return count;
}

int GridMap::obstacleCount() const {
  int count = 0;
  for (const auto& entry : cells) {
    if (entry.second == OBSTACULO) {
      ++count;
    }
  }
  return count;
}

int GridMap::unknownCount() const {
  int minX, minY, maxX, maxY;
  bounds(minX, minY, maxX, maxY);
  int count = 0;
  for (int gy = minY; gy <= maxY; ++gy) {
    for (int gx = minX; gx <= maxX; ++gx) {
      if (cellAt(gx, gy) == DESCONHECIDO) {
        ++count;
      }
    }
  }
  return count;
}

void GridMap::bounds(int& minX, int& minY, int& maxX, int& maxY) const {
  minX = maxX = wx;
  minY = maxY = wy;

  for (const auto& entry : cells) {
    const int gx = entry.first.first;
    const int gy = entry.first.second;
    if (gx < minX) minX = gx;
    if (gx > maxX) maxX = gx;
    if (gy < minY) minY = gy;
    if (gy > maxY) maxY = gy;
  }

  for (int i = 0; i < pathLen; ++i) {
    if (pathX[i] < minX) minX = pathX[i];
    if (pathX[i] > maxX) maxX = pathX[i];
    if (pathY[i] < minY) minY = pathY[i];
    if (pathY[i] > maxY) maxY = pathY[i];
  }
}

void GridMap::printMap() const {
  int minX, minY, maxX, maxY;
  bounds(minX, minY, maxX, maxY);

  Serial.println();
  Serial.println("=========== MAPA ===========");

  for (int gy = minY; gy <= maxY; ++gy) {
    for (int gx = minX; gx <= maxX; ++gx) {
      if (gx == wx && gy == wy) {
        Serial.print('R');
        continue;
      }

      switch (cellAt(gx, gy)) {
        case LIVRE:
          Serial.print('o');
          break;
        case OBSTACULO:
          Serial.print('#');
          break;
        default:
          Serial.print('.');
          break;
      }
    }
    Serial.println();
  }

  Serial.print("Visitadas=");
  Serial.print(visitedCount());
  Serial.print(" Obstaculos=");
  Serial.print(obstacleCount());
  Serial.print(" Extensao=");
  Serial.print(maxX - minX + 1);
  Serial.print("x");
  Serial.print(maxY - minY + 1);
  Serial.print(" Posicao=(");
  Serial.print(wx);
  Serial.print(",");
  Serial.print(wy);
  Serial.print(") Direcao=");
  Serial.println(headingChar(dir));

  Serial.println("============================");
  Serial.println();
}
