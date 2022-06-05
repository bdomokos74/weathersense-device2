#include "bme_sensor.h"
#include "SerialLogger.h"
#include <Arduino.h>

Adafruit_Sensor *bme_temp;
Adafruit_Sensor *bme_pressure;
Adafruit_Sensor *bme_humidity;

bool bmeFound = false;

void _init(Adafruit_BME280 *bme, int addr) {
  if(!bme->begin(addr)) {
    Serial.print("Did not find BME280 sensor, 0x");
    Serial.println(addr, 16);
    // 0xFF - bad address, 0x56-58 - BMP280, 0x60 - BME280, 0x61 - BME680
  } else {
    Serial.print("BME found: 0x");
    Serial.println(bme->sensorID(), 16);
    bmeFound = true;    
    bme_temp = bme->getTemperatureSensor();
    bme_pressure = bme->getPressureSensor();
    bme_humidity = bme->getHumiditySensor();
  }
}

BMESensor::BMESensor(int addr) {
  Serial.println("BMESensor constructor");
  Adafruit_BME280 *bme = new Adafruit_BME280();
  _init(bme, addr);
}



bool BMESensor::isConnected() {
  return bmeFound;
}

float BMESensor::readTemp() {
  sensors_event_t event;
  if(bme_temp != NULL) {
    bme_temp->getEvent(&event);
  }
  return event.temperature;
}

float BMESensor::readPressure() {
  sensors_event_t event;
  if(bme_pressure != NULL) {
    bme_pressure->getEvent(&event);
  }
  return event.pressure;
  //return bme_pressure->read()/100.0F;
}

float BMESensor::readHumidity() {
  sensors_event_t event;
  if(bme_humidity != NULL) {
    bme_humidity->getEvent(&event);
  }
  return event.relative_humidity;
  //return bme.readHumidity();
}

void BMESensor::printTelemetry() {
  Logger.Info("temp="+String(this->readTemp()));
  Logger.Info("pressure="+String(this->readPressure()));
  Logger.Info("h="+String(this->readHumidity()));
}

void BMESensor::debug() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);          
}