#ifndef _HEADER_wifi
#define _HEADER_wifi

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "Mutexed.h"

namespace blastic {

/*
  WifiConnection represent a connection to a WiFi AP. It acts as a mutex
  in relation to the WiFi global object.

  The underlying WiFi connection is not destroyed immediately after this
  object goes out of scope, but it is kept around for a configurable timeout.
*/

class WifiConnection : public util::Mutexed<WiFi> {
public:
  static const bool ipConnectBroken;

  struct [[gnu::packed]] EEPROMConfig {
    // leave the password empty to connect to an open network
    char ssid[32], password[64];
    unsigned long dhcpTimeout, disconnectTimeout;
  };

  WifiConnection(const EEPROMConfig &config);
  // was the connection successful?
  operator bool() const;
  ~WifiConnection();
};

/*
  Clients use a pseudo socket system, but the Arduino class never closes it...
  Redefine the WiFiSSLClient type and ensure that the socket is closed on destruction.
*/
class WiFiSSLClient : public ::WiFiSSLClient {
public:
  using ::WiFiSSLClient::WiFiSSLClient;
  ~WiFiSSLClient() { stop(); }
};

} // namespace blastic

#endif
