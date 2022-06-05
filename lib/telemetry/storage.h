#ifndef MSTORAGE_H
#define MSTORAGE_H
#ifdef xxxyyyy
#include "bme_sensor.h"
#include "dallas_sensor.h"
#include "gm_sensor.h"
#include "az_span.h"

class Storage {
private:
  BMESensor *bmeSensor;
  //DallasSensor *dallasSensor;
  GMSensor *gmSensor;
  State *deviceState;
public:
  Storage(BMESensor *bme, GMSensor *gm, State *state);
  int storeMeasurement();
  void printStatus();
  void reset();
  int getNumStoredMeasurements();
  char *getDataBuf();
  int getMeasurementString(char * buf, int size);
  bool isBufferFull();
  int getRemainingLen();
};

#endif
#endif