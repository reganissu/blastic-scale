#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "blastic.h"
#include "SerialCliTask.h"
#include "Scale.h"
#include "Display.h"
#include "WifiConnection.h"

namespace blastic {

struct [[gnu::packed]] EEPROMConfig {
  Scale::EEPROMConfig scale;
  WifiConnection::EEPROMConfig wifi;
};

static constexpr const char PROGMEM version[] = {GIT_COMMIT " worktree " GIT_WORKTREE_STATUS};
static constexpr const EEPROMConfig defaultConfig{.scale = {.dataPin = 2, .clockPin = 3, .scale = 1.0}};

Scale scale(defaultConfig.scale);

namespace cli {

static void version(WordSplit &) {
  MSerial serial;
  serial->print(F("version: "));
  serial->println(F(blastic::version));
}

static void echo(WordSplit &args) {
  MSerial serial;
  serial->print(F("echo:"));
  for (char *arg = args.nextWord(); arg; arg = args.nextWord()) {
    serial->print(' ');
    serial->print(arg);
  }
  serial->println();
}

namespace scale {

static void weight(WordSplit &) {
  auto value = blastic::scale.read();
  MSerial serial;
  serial->print(F("scale::weight: "));
  serial->println(value);
}

} // namespace scale

namespace wifi {

static void status(WordSplit &args) {
  MSerial serial;
  if (args.nextWordIs("debug")) modem.debug(*serial, 2);
  MWiFi wifi;
  auto status = wifi->status();
  auto wifiVersion = wifi->firmwareVersion();
  modem.noDebug();
  serial->print(F("wifi::status: status "));
  serial->print(status);
  serial->print(F(" version "));
  serial->println(wifiVersion);
}

static void connect(WordSplit &args) {
  auto ssid = args.nextWord();
  auto password = args.nextWord();
  MSerial serial;
  auto outPrefix = F("wifi::connect: ");
  if (!ssid) {
    serial->print(outPrefix);
    serial->print(F("missing ssid argument\n"));
    return;
  }
  WifiConnection::EEPROMConfig config;
  config.timeoutSec = 10;
  strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
  if (password) strncpy(config.password, password, sizeof(config.password) - 1);
  if (args.nextWordIs("debug")) modem.debug(*serial, 2);

  serial->print(outPrefix);
  serial->print(F("begin connection to "));
  serial->println(config.ssid);
  WifiConnection wifi(config);
  auto status = wifi->status();
  serial->print(outPrefix);
  if (status != WL_CONNECTED) {
    modem.noDebug();
    serial->print(F("connection failed ("));
    serial->print(status);
    serial->println(')');
    return;
  }
  serial->print(F("connected\n"));

  uint8_t bssid[6];
  wifi->BSSID(bssid);
  auto rssi = wifi->RSSI();
  auto ip = wifi->localIP(), gateway = wifi->gatewayIP(), dns1 = wifi->dnsIP(0), dns2 = wifi->dnsIP(1);
  modem.noDebug();

  serial->print(outPrefix);
  serial->print(F("bssid "));
  for (auto b : bssid) serial->print(b, 16);
  serial->print(F(" rssi "));
  serial->print(rssi);
  serial->print(F("dBm ip "));
  ip.printTo(*serial);
  serial->print(F(" gateway "));
  gateway.printTo(*serial);
  serial->print(F(" dns1 "));
  dns1.printTo(*serial);
  serial->print(F(" dns2 "));
  dns2.printTo(*serial);
  serial->println();
}

} // namespace wifi

static constexpr const CliCallback callbacks[]{makeCliCallback(version),       makeCliCallback(echo),
                                               makeCliCallback(scale::weight), makeCliCallback(wifi::status),
                                               makeCliCallback(wifi::connect), CliCallback()};

} // namespace cli

} // namespace blastic

void setup() {
  using namespace blastic;
  // some gracetime to start `platformio device monitor` after upload or power on
  delay(1000);
  {
    MSerial serial;
    serial->begin(9600);
    while (!*serial);
    serial->print(F("Booting blastic-scale version "));
    serial->println(F(version));
    // use 4 KiB of stack, this was seen to trigger a stack overflow in wifi functions
    static cli::SerialCliTask<Serial, 4096> cli(cli::callbacks);
    static util::StaticTask display(ui::loop, "DisplayTask");
    serial->print(F("Starting FreeRTOS scheduler.\n"));
  }
  vTaskStartScheduler();

  for (;;);
}
