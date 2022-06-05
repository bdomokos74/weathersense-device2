#include "props.h"
#include "esp_task_wdt.h"

RTC_DATA_ATTR int doSleep = 0;
RTC_DATA_ATTR int sleepTimeSec = 0;
RTC_DATA_ATTR int measureIntervalMs = 0;
RTC_DATA_ATTR int mesaureBatchSize = 0;
RTC_DATA_ATTR int ledPin = 0;

Props::Props(char *fw, char *gitRev) {
    this->firmwareVersion = fw;
    this->gitRevision = gitRev;
}

char* Props::getFirmwareVersion() {
    return this->firmwareVersion;
}

char* Props::getGitRevision() {
    return this->gitRevision;
}

int Props::getDoSleep(){
    return doSleep;
}

int Props::getSleepTimeSec(){
    return sleepTimeSec;
}

int Props::getMeasureIntervalMs(){
    return measureIntervalMs;
}

int Props::getMeasureBatchSize(){
    return mesaureBatchSize;
}

int Props::getLedPin(){
    return ledPin;
}

