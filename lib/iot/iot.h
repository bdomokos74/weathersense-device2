#ifndef _WIOT_H
#define _WIOT_H

void initializeIoTHubClient();
int initializeMqttClient();

int sendTelemetry();
void sendTwinProp();
bool requestTwin();

#endif
