#include "GridMap.h"

#include <Arduino.h>

void GridMap::begin() {
  reset();
}

void GridMap::reset() {
  for (int r = 0; r < TAMANHO; ++r) {
    for (int c = 0; c < TAMANHO; ++c) {
      cells[r][c] = DESCONHECIDO;
      visits[r][c] = 0;
    }
  }
  posX = ORIGEM;
  posY = ORIGEM;
  dir = 0;
  markVisited();
}

int GridMap::x() const {
  return posX;
}

int GridMap::y() const {
  return posY;
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

bool GridMap::inBounds(int gx, int gy) {
  return gx >= 0 && gx < TAMANHO && gy >= 0 && gy < TAMANHO;
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

// O robo esteve aqui, entao a celula e livre mesmo que antes tenha sido marcada
// como obstaculo (a pose estimada por tempo deriva).
char GridMap::robotChar(int direction) {
  switch (direction) {
    case 0:
      return '^';
    case 1:
      return '>';
    case 2:
      return 'v';
    default:
      return '<';
  }
}

void GridMap::markVisited() {
  if (inBounds(posX, posY)) {
    cells[posY][posX] = LIVRE;
    if (visits[posY][posX] < 255) {
      ++visits[posY][posX];
    }
  }
}

int GridMap::visitsHere() const {
  return visits[posY][posX];
}

int GridMap::visitsAhead(int direction) const {
  int dx, dy;
  dxdy(direction, dx, dy);

  int total = 0;
  for (int i = 1; i <= ALCANCE_AVALIACAO; ++i) {
    const int gx = posX + dx * i;
    const int gy = posY + dy * i;
    if (!inBounds(gx, gy)) {
      break;
    }
    total += visits[gy][gx];
  }
  return total;
}

void GridMap::markObstacle(int direction) {
  int dx, dy;
  dxdy(direction, dx, dy);

  const int gx = posX + dx;
  const int gy = posY + dy;

  if (inBounds(gx, gy) && cells[gy][gx] == DESCONHECIDO) {
    cells[gy][gx] = OBSTACULO;
  }
}

void GridMap::stepForward() {
  int dx, dy;
  dxdy(dir, dx, dy);

  posX += dx;
  posY += dy;

  if (!inBounds(posX, posY)) {
    posX -= dx;
    posY -= dy;
    return;
  }

  markVisited();
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
    const int gx = posX + dx * i;
    const int gy = posY + dy * i;
    if (!inBounds(gx, gy)) {
      break;
    }
    if (cells[gy][gx] == DESCONHECIDO) {
      ++count;
    }
  }
  return count;
}

int GridMap::visitedCount() const {
  int count = 0;
  for (int r = 0; r < TAMANHO; ++r) {
    for (int c = 0; c < TAMANHO; ++c) {
      if (cells[r][c] == LIVRE) {
        ++count;
      }
    }
  }
  return count;
}

int GridMap::obstacleCount() const {
  int count = 0;
  for (int r = 0; r < TAMANHO; ++r) {
    for (int c = 0; c < TAMANHO; ++c) {
      if (cells[r][c] == OBSTACULO) {
        ++count;
      }
    }
  }
  return count;
}

int GridMap::unknownCount() const {
  int count = 0;
  for (int r = 0; r < TAMANHO; ++r) {
    for (int c = 0; c < TAMANHO; ++c) {
      if (cells[r][c] == DESCONHECIDO) {
        ++count;
      }
    }
  }
  return count;
}

void GridMap::printMap() const {
  Serial.println();
  Serial.println("=========== MAPA ===========");

  for (int r = 0; r < TAMANHO; ++r) {
    for (int c = 0; c < TAMANHO; ++c) {
      if (c == posX && r == posY) {
        Serial.print(robotChar(dir));
        continue;
      }

      switch (cells[r][c]) {
        case LIVRE:
          // Numero de passagens pela celula (9 = nove ou mais).
          Serial.print(visits[r][c] > 9 ? 9 : visits[r][c]);
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
  Serial.print(" Desconhecidas=");
  Serial.print(unknownCount());
  Serial.print(" Posicao=(");
  Serial.print(posX - ORIGEM);
  Serial.print(",");
  Serial.print(ORIGEM - posY);
  Serial.print(") Direcao=");
  Serial.println(headingChar(dir));

  Serial.println("============================");
  Serial.println();
}
