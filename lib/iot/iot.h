#ifndef _WIOT_H
#define _WIOT_H


#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60

void initializeIoTHubClient();
int initializeMqttClient();

int sendTelemetry();
void sendTwinProp();
bool requestTwin();

#endif
