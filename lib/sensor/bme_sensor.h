#ifndef BMEUTIL_H
#define BMEUTIL_H

#include <Adafruit_BME280.h>

class BMESensor {
private:
  bool bmeFound = false;
public:
  BMESensor();
  BMESensor(int addr);
  bool isConnected();
  float readTemp();
  float readPressure();
  float readHumidity();
  void printTelemetry();
  static void debug();
};

#endif
