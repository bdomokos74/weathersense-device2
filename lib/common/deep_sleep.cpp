#include "deep_sleep.h"
#include <SerialLogger.h>

#define LOOP_TIME_MILLIS 10000
#define sleepTimeSec 120

RTC_DATA_ATTR int wakeCnt = 0;


DeepSleep::DeepSleep() {
}

void DeepSleep::incrementCount() {
  wakeCnt++;
}

int DeepSleep::getWakeCount() {
  return wakeCnt;
}

void DeepSleep::logWakeup() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  switch(reason) {
  case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", reason); break;
  }
}

bool DeepSleep::isWakeup() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  return (reason==ESP_SLEEP_WAKEUP_TIMER);
}

void DeepSleep::goSleep(int sleepSec) {
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepSec);
  esp_deep_sleep_start();
} 

void DeepSleep::goSleep() 
{
  Logger.Info("go to sleep");
  ++wakeCnt;
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * sleepTimeSec);
  esp_deep_sleep_start();
}
