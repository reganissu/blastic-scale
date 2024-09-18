#include "blastic.h"
#include "StaticTask.h"

namespace blastic {

static unsigned long lastUnusedTime = 0, timeoutSec = 0;

static void timeoutDisconnect() {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    MWiFi wifi;
    if (!lastUnusedTime || millis() - lastUnusedTime < timeoutSec * 1000) continue;
    wifi->end();
    lastUnusedTime = 0;
    MSerial()->print(F("wifi: disconneted\n"));
  }
}

WifiConnection::WifiConnection(const EEPROMConfig &config) : util::Mutexed<WiFi>() {
  MWiFi &wifi = *this;
  configASSERT(!strcmp(wifi->firmwareVersion(), WIFI_FIRMWARE_LATEST_VERSION));
  timeoutSec = config.timeoutSec;
  if (*this && !strncmp(wifi->SSID(), config.ssid, sizeof(config.ssid))) return;
  wifi->end();
  wifi->begin(config.ssid, strnlen(config.password, sizeof(config.password)) ? config.password : nullptr);
}

WifiConnection::operator bool() const { return (*this)->status() == WL_CONNECTED; }

WifiConnection::~WifiConnection() {
  lastUnusedTime = millis();
  static util::StaticTask<1024> timeoutTask(timeoutDisconnect, "WiFiTimeout");
}

} // namespace blastic
