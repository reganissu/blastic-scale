#include <iterator>
#include "blastic.h"
#include "SerialCliTask.h"

namespace blastic {

static constexpr const char version[] = {GIT_COMMIT " worktree " GIT_WORKTREE_STATUS};

// initialize configuration with sane defaults
EEPROMConfig config = {.scale = {.dataPin = 2,
                                 .clockPin = 3,
                                 .mode = scale::HX711Mode::A128,
                                 .calibrations = {{.tareRawRead = 0, .weightRawRead = 0, .weight = 0.f},
                                                  {.tareRawRead = 0, .weightRawRead = 0, .weight = 0.f},
                                                  {.tareRawRead = 0, .weightRawRead = 0, .weight = 0.f}}},
                       // XXX GCC bug, cannot use initializer lists with strings
                       .wifi = WifiConnection::EEPROMConfig{"unconfigured-ssid", "unconfigured-password", 10}};

bool debug = false;

namespace cli {

static void version(WordSplit &) {
  MSerial serial;
  serial->print("version: ");
  serial->println(F(blastic::version));
}

static void debug(WordSplit &args) {
  blastic::debug = args.nextWordIs("on");
  if (debug) modem.debug(Serial, 2);
  else modem.noDebug();
  MSerial serial;
  serial->print("debug: ");
  serial->print(blastic::debug ? "on\n" : "off\n");
}

static void echo(WordSplit &args) {
  MSerial serial;
  serial->print("echo:");
  for (char *arg = args.nextWord(); arg; arg = args.nextWord()) {
    serial->print(' ');
    serial->print(arg);
  }
  serial->println();
}

namespace scale {

using namespace blastic::scale;
constexpr const uint32_t scaleCliTimeout = 2000, scaleCliMaxMedianWidth = 16;

static void mode(WordSplit &args) {
  auto modeString = args.nextWord();
  MSerial serial;
  serial->print("scale::mode: ");
  if (!modeString) {
    serial->print("missing mode argument\n");
    return;
  }
  for (uint8_t i = 0; i < std::size(HX711ModeStrings); i++) {
    if (strcmp(modeString, HX711ModeStrings[i])) continue;
    config.scale.mode = HX711Mode(i);
    serial->println(modeString);
    return;
  }
  serial->print("invalid\n");
}

static void tare(WordSplit &) {
  auto value = raw(config.scale, scaleCliMaxMedianWidth, pdMS_TO_TICKS(scaleCliTimeout));
  MSerial serial;
  serial->print("scale::tare: ");
  if (value == invalidRead) {
    serial->print("failed to get measurements for tare\n");
    return;
  }
  auto &calibration = config.scale.calibrations[uint8_t(config.scale.mode)];
  calibration.tareRawRead = value;
  serial->println(value);
}

static void calibrate(WordSplit &args) {
  auto weightString = args.nextWord();
  if (!weightString) {
    MSerial()->print("scale::calibrate: missing test weight argument\n");
    return;
  }
  char *weightEnd;
  auto weight = strtof(weightString, &weightEnd);
  if (weightString == weightEnd) {
    MSerial()->print("scale::calibrate: cannot parse test weight argument\n");
    return;
  }
  auto value = raw(config.scale, scaleCliMaxMedianWidth, pdMS_TO_TICKS(scaleCliTimeout));
  MSerial serial;
  serial->print("scale::calibrate: ");
  if (value == invalidRead) {
    serial->print("failed to get measurements for calibration\n");
    return;
  }
  auto &calibration = config.scale.calibrations[uint8_t(config.scale.mode)];
  calibration.weightRawRead = value, calibration.weight = weight;
  serial->println(value);
}

static void raw(WordSplit &args) {
  auto medianWidthArg = args.nextWord();
  auto medianWidth = min(max(1, medianWidthArg ? atoi(medianWidthArg) : 1), scaleCliMaxMedianWidth);
  auto value = blastic::scale::raw(config.scale, medianWidth, pdMS_TO_TICKS(scaleCliTimeout));
  MSerial serial;
  serial->print("scale::raw: ");
  value == invalidRead ? serial->print("invalid\n") : serial->println(value);
}

static void weight(WordSplit &args) {
  auto &calibration = config.scale.calibrations[uint8_t(config.scale.mode)];
  if (calibration.weightRawRead - calibration.tareRawRead == 0) {
    MSerial()->print("scale::weight: cannot calculate weight without calibration parameters\n");
    return;
  }
  auto medianWidthArg = args.nextWord();
  auto medianWidth = min(max(1, medianWidthArg ? atoi(medianWidthArg) : 1), scaleCliMaxMedianWidth);
  auto value = blastic::scale::weight(config.scale, medianWidth, pdMS_TO_TICKS(scaleCliTimeout));
  MSerial serial;
  serial->print("scale::weight: ");
  isnan(value) ? serial->print("invalid\n") : serial->println(value);
}

} // namespace scale

namespace wifi {

static void status(WordSplit &args) {
  uint8_t status;
  char firmwareVersion[12];
  {
    MWiFi wifi;
    status = wifi->status();
    strncpy(firmwareVersion, wifi->firmwareVersion(), sizeof(firmwareVersion) - 1);
  }
  MSerial serial;
  serial->print("wifi::status: status ");
  serial->print(status);
  serial->print(" version ");
  serial->println(firmwareVersion);
}

static void timeout(WordSplit &args) {
  if (auto timeoutString = args.nextWord()) {
    char *timeoutEnd;
    auto timeout = strtoul(timeoutString, &timeoutEnd, 10);
    if (timeoutString != timeoutEnd) config.wifi.timeoutSec = timeout;
  }
  MSerial serial;
  serial->print("wifi::timeout: ");
  serial->println(config.wifi.timeoutSec);
}

static void connect(WordSplit &args) {
  auto ssid = args.nextWord();
  auto password = args.nextWord();
  if (!ssid) {
    MSerial()->print("wifi::connect: missing ssid argument\n");
    return;
  }
  WifiConnection::EEPROMConfig config = blastic::config.wifi;
  strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
  if (password) strncpy(config.password, password, sizeof(config.password) - 1);
  else config.password[0] = '\0';

  {
    MSerial serial;
    serial->print("wifi::connect: begin connection to ");
    serial->println(config.ssid);
  }

  uint8_t bssid[6];
  int32_t rssi;
  IPAddress ip, gateway, dns1, dns2;
  {
    WifiConnection wifi(config);
    auto status = wifi->status();
    if (status != WL_CONNECTED) {
      MSerial serial;
      serial->print("wifi::connect: connection failed (");
      serial->print(status);
      serial->println(')');
      return;
    }
    MSerial()->print("wifi::connect: connected\n");
    wifi->BSSID(bssid);
    rssi = wifi->RSSI();
    ip = wifi->localIP(), gateway = wifi->gatewayIP(), dns1 = wifi->dnsIP(0), dns2 = wifi->dnsIP(1);
  }

  MSerial serial;
  serial->print("wifi::connect: bssid ");
  for (auto b : bssid) serial->print(b, 16);
  serial->print(" rssi ");
  serial->print(rssi);
  serial->print("dBm ip ");
  serial->print(ip);
  serial->print(" gateway ");
  serial->print(gateway);
  serial->print(" dns1 ");
  serial->print(dns1);
  serial->print(" dns2 ");
  serial->println(dns2);
}

} // namespace wifi

static constexpr const CliCallback callbacks[]{makeCliCallback(version),
                                               makeCliCallback(debug),
                                               makeCliCallback(echo),
                                               makeCliCallback(scale::mode),
                                               makeCliCallback(scale::tare),
                                               makeCliCallback(scale::calibrate),
                                               makeCliCallback(scale::raw),
                                               makeCliCallback(scale::weight),
                                               makeCliCallback(wifi::status),
                                               makeCliCallback(wifi::connect),
                                               CliCallback()};

} // namespace cli

} // namespace blastic

void setup() [[noreturn]] {
  using namespace blastic;
  // some gracetime to start `platformio device monitor` after upload or power on
  delay(1000);
  // XXX do not use FreeRTOS functions other than allocating tasks, crashes seen otherwise
  Serial.begin(9600);
  while (!Serial);
  Serial.print("Booting blastic-scale version ");
  Serial.println(F(version));
  // use 4 KiB of stack, this was seen to trigger a stack overflow in wifi functions
  static cli::SerialCliTask<Serial, 4096> cli(cli::callbacks);
  static util::StaticTask display(ui::loop, "DisplayTask");
  Serial.print("Starting FreeRTOS scheduler.\n");
  vTaskStartScheduler();
}
