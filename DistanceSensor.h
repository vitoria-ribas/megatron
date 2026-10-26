#pragma once

class DistanceSensor {
 public:
  void begin();
  float readOnce();
  float readFiltered();

 private:
  unsigned long lastTriggerMs = 0;
};
