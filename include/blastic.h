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

extern Scale scale;

// take these mutexes to access a global device
using MSerial = util::Mutexed<::Serial>;
using MScale = util::Mutexed<scale>;
using MWiFi = util::Mutexed<::WiFi>;

} // namespace blastic

#endif
