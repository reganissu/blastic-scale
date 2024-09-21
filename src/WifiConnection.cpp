#include "blastic.h"
#include "StaticTask.h"

namespace blastic {

static unsigned long lastUnusedTime = 0, disconnectTimeout = 0;

static void timeoutDisconnect() {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    {
      MWiFi wifi;
      if (!lastUnusedTime || millis() - lastUnusedTime < disconnectTimeout * 1000) continue;
      wifi->end();
      lastUnusedTime = 0;
    }
    MSerial()->print("wifi: disconneted\n");
  }
}

WifiConnection::WifiConnection(const EEPROMConfig &config) : util::Mutexed<WiFi>() {
  constexpr const uint32_t dhcpPollInterval = 100;
  auto &wifi = *this;
  configASSERT(!strcmp(wifi->firmwareVersion(), WIFI_FIRMWARE_LATEST_VERSION));
  disconnectTimeout = config.disconnectTimeout;
  if (*this && !strncmp(wifi->SSID(), config.ssid, sizeof(config.ssid))) return;
  wifi->end();
  wifi->begin(config.ssid, strnlen(config.password, sizeof(config.password)) ? config.password : nullptr);
  auto dhcpStart = millis();
  while (!wifi->localIP() && millis() - dhcpStart < config.dhcpTimeout * 1000) vTaskDelay(dhcpPollInterval);
}

WifiConnection::operator bool() const {
  auto &_this = *this;
  return _this->status() == WL_CONNECTED && _this->localIP() &&  _this->gatewayIP();
 }

WifiConnection::~WifiConnection() {
  lastUnusedTime = millis();
  static util::StaticTask<1024> timeoutTask(timeoutDisconnect, "WiFiTimeout");
}

} // namespace blastic
