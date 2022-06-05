#ifndef WIFINET_H
#define WIFINET_H

#include <WiFi.h>

class WifiNet {
private:

public:
  WifiNet(char *ssid, char *pw);
  bool connect();
  bool isConnected();
  bool initializeTime();
  void close();
};

#endif
