#ifndef WIFINET_H
#define WIFINET_H

#include <WiFi.h>
#include <time.h>
#include <log.h>

class WifiNet {
private:

public:
  WifiNet();
  bool connect();
  bool isConnected();
  void close();
};

#endif
