#pragma once

#ifndef BLASTIC_MONITOR_SPEED
#error Define the serial baud rate in BLASTIC_MONITOR_SPEED
#endif

// environment
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Mutexed.h"

// classes / task functions for devices
#include "Scale.h"
#include "WifiConnection.h"
#include "Buttons.h"
#include "Submitter.h"

namespace blastic {

constexpr const char version[] = {BLASTIC_GIT_COMMIT " worktree " BLASTIC_GIT_WORKTREE_STATUS};

extern uint32_t debug;

struct [[gnu::packed]] EEPROMConfig {
  scale::EEPROMConfig scale;
  WifiConnection::EEPROMConfig wifi;
  blastic::Submitter::EEPROMConfig submit;
  buttons::EEPROMConfig buttons;
};

extern EEPROMConfig config;

// take these mutexes to access a global device
using MSerial = util::Mutexed<::Serial>;
using MWiFi = util::Mutexed<::WiFi>;

} // namespace blastic
