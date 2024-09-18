#ifndef _HEADER_blastic
#define _HEADER_blastic

// environment
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Mutexed.h"

// classes / task functions for devices
#include "Scale.h"
#include "WifiConnection.h"
#include "Display.h"

namespace blastic {

extern bool debug;

struct [[gnu::packed]] EEPROMConfig {
  scale::EEPROMConfig scale;
  WifiConnection::EEPROMConfig wifi;
};

extern EEPROMConfig config;

// take these mutexes to access a global device
using MSerial = util::Mutexed<::Serial>;
using MWiFi = util::Mutexed<::WiFi>;

} // namespace blastic

#endif
