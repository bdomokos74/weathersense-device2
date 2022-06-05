#ifndef _DEEP_SLEEP_H
#define _DEEP_SLEEP_H

#include "esp_task_wdt.h"

#define uS_TO_S_FACTOR 1000000

class DeepSleep {
private:

public:
  DeepSleep();
  void logWakeup(); 
  bool isWakeup();
  void incrementCount();
  int getWakeCount();
  void goSleep();
  void goSleep(int sleepSec);
};

#endif
