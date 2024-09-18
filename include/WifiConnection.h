#ifndef _HEADER_wifi
#define _HEADER_wifi

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "Mutexed.h"

namespace blastic {

class WifiConnection : public util::Mutexed<WiFi> {
public:
  struct [[gnu::packed]] EEPROMConfig {
    char ssid[32], password[64];
    unsigned long timeoutSec;
  };

  WifiConnection(const EEPROMConfig &config);
  operator bool() const;
  ~WifiConnection();
};

} // namespace blastic

#endif
