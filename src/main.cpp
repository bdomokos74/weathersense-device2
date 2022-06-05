/**
 * A simple Azure IoT example for sending telemetry to Iot Hub.
 */

#include <SerialLogger.h>
#include <wifinet.h>
#include <local_config.h>

//#include "caw.h"

#include <led.h>
#include <deep_sleep.h>
#include <bme_sensor.h>

#include "esp_task_wdt.h"
#include "esp_system.h"

#include "iot.h"

#include "telemetry.h"

unsigned long start_interval_ms = 0;
unsigned long lastSend = 0;
unsigned long loopCnt = 0;
unsigned long start_millis;

#define MEAS_INTERVAL_MILLIS 30000

RTC_DATA_ATTR bool prevConnFailed = false;

WifiNet *wifiNet;
BMESensor *bmeSensor;
DeepSleep *deepSleep;

char *iothubHost = IOT_CONFIG_IOTHUB_FQDN;
char *mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
char *iotDeviceId = IOT_CONFIG_DEVICE_ID;
char *iotDeviceKey = IOT_CONFIG_DEVICE_KEY;

void establishConnection()
{
  // defined in platformio.ini, via env vars
  wifiNet = new WifiNet(IOT_CONFIG_WIFI_SSID, IOT_CONFIG_WIFI_PASSWORD);
  if (!wifiNet->connect())
  {
    deepSleep->goSleep(120);
  }
  if (!wifiNet->initializeTime())
  {
    wifiNet->close();
    deepSleep->goSleep(120);
  }

  initializeIoTHubClient();
  initializeMqttClient();
  start_millis = millis();
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  while (!Serial)
  {
  };

  delay(100);
  Logger.info("ESP32 Device Initializing...");

  esp_log_level_set("*", ESP_LOG_DEBUG);

  start_interval_ms = millis();
  Wire.begin();

  bmeSensor = new BMESensor(BME_ADDR);
  deepSleep = new DeepSleep();

  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  establishConnection();

  loopCnt = 0;
  lastSend = millis();
}

bool twinDone = false;
void loop()
{

  if (loopCnt == 0 || (int)(millis() - lastSend) > MEAS_INTERVAL_MILLIS)
  {
    loopCnt++;
    // readSensor();
    // BMESensor::debug();
    int ret = sendTelemetry();
    if(ret!=0) {
      esp_task_wdt_reset();
    }

    lastSend = millis();
  }

  if (!twinDone && (int)(millis() - start_millis) > 5000)
  {
    // sendTwinProp();
    requestTwin();
    twinDone = true;
  }
}
