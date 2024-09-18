#include "blastic.h"
#include "SerialCliTask.h"

namespace blastic {

static constexpr const char PROGMEM version[] = {GIT_COMMIT " worktree " GIT_WORKTREE_STATUS};

// initialize configuration with sane defaults
EEPROMConfig config{.scale = {.dataPin = 2,
                              .clockPin = 3,
                              .chanAMode = scale::gain_t::GAIN_128,
                              .calibrationChanA128 = {.tareRawRead = 0, .weightRawRead = 0, .weight = 0.f},
                              .calibrationChanA64 = {.tareRawRead = 0, .weightRawRead = 0, .weight = 0.f}},
                    .wifi = WifiConnection::EEPROMConfig{"unconfigured-ssid", "unconfigured-password", 10}};

bool debug = false;

namespace cli {

static void version(WordSplit &) {
  MSerial serial;
  serial->print(F("version: "));
  serial->println(F(blastic::version));
}

static void debug(WordSplit &args) {
  blastic::debug = args.nextWordIs("on");
  MSerial serial;
  serial->print(F("debug: "));
  serial->println(blastic::debug ? F("on") : F("off"));
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

static void raw(WordSplit &) {
  auto value = blastic::scale::raw(config.scale, 1, pdMS_TO_TICKS(1000));
  MSerial serial;
  serial->print(F("scale::raw: "));
  value == blastic::scale::invalidRead ? serial->println(F("invalid")) : serial->println(value);
}

static void weight(WordSplit &args) {
  auto medianWidthArg = args.nextWord();
  auto medianWidth = min(max(1, medianWidthArg ? atoi(medianWidthArg) : 1), 16);
  auto value = blastic::scale::weight(config.scale, medianWidth, pdMS_TO_TICKS(1000));
  MSerial serial;
  serial->print(F("scale::weight: "));
  value == blastic::scale::invalidWeight ? serial->println(F("invalid")) : serial->println(value);
}

} // namespace scale

namespace wifi {

static void status(WordSplit &args) {
  uint8_t status;
  char firmwareVersion[12];
  {
    MWiFi wifi;
    MSerial serial(debug);
    if (serial.taken()) modem.debug(*serial, 2);
    status = wifi->status();
    strncpy(firmwareVersion, wifi->firmwareVersion(), sizeof(firmwareVersion) - 1);
    modem.noDebug();
  }
  MSerial serial;
  serial->print(F("wifi::status: status "));
  serial->print(status);
  serial->print(F(" version "));
  serial->println(firmwareVersion);
}

static void connect(WordSplit &args) {
  auto ssid = args.nextWord();
  auto password = args.nextWord();
  auto outPrefix = F("wifi::connect: ");
  if (!ssid) {
    MSerial serial;
    serial->print(outPrefix);
    serial->print(F("missing ssid argument\n"));
    return;
  }
  WifiConnection::EEPROMConfig config = blastic::config.wifi;
  strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
  if (password) strncpy(config.password, password, sizeof(config.password) - 1);
  else config.password[0] = '\0';

  {
    MSerial serial;
    serial->print(outPrefix);
    serial->print(F("begin connection to "));
    serial->println(config.ssid);
  }

  uint8_t bssid[6];
  int32_t rssi;
  IPAddress ip, gateway, dns1, dns2;
  {
    WifiConnection wifi(config);
    MSerial serial(debug);
    if (serial.taken()) modem.debug(*serial, 2);
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
    wifi->BSSID(bssid);
    rssi = wifi->RSSI();
    ip = wifi->localIP(), gateway = wifi->gatewayIP(), dns1 = wifi->dnsIP(0), dns2 = wifi->dnsIP(1);
    modem.noDebug();
  }

  MSerial serial;
  serial->print(outPrefix);
  serial->print(F("bssid "));
  for (auto b : bssid) serial->print(b, 16);
  serial->print(F(" rssi "));
  serial->print(rssi);
  serial->print(F("dBm ip "));
  serial->print(ip);
  serial->print(F(" gateway "));
  serial->print(gateway);
  serial->print(F(" dns1 "));
  serial->print(dns1);
  serial->print(F(" dns2 "));
  serial->println(dns2);
}

} // namespace wifi

static constexpr const CliCallback callbacks[]{makeCliCallback(version),       makeCliCallback(debug),
                                               makeCliCallback(echo),          makeCliCallback(scale::raw),
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
