#include <iterator>
#include "blastic.h"
#include "WiFiSSLClient.h"
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
                       .wifi = WifiConnection::EEPROMConfig{"unconfigured-ssid", "unconfigured-password", 10, 10}};

bool debug = false;

namespace cli {

static void version(WordSplit &) {
  MSerial serial;
  serial->print("version: ");
  serial->println(blastic::version);
}

static void uptime(WordSplit &) {
  auto s = millis() / 1000;
  MSerial serial;
  serial->print("uptime: ");
  serial->print(s / 60 / 60 / 24);
  serial->print('d');
  serial->print(s / 60 / 60);
  serial->print('h');
  serial->print(s / 60);
  serial->print('m');
  serial->print(s);
  serial->print("s\n");
}

static void debug(WordSplit &args) {
  blastic::debug = args.nextWordIs("on");
  if (blastic::debug) modem.debug(Serial, 2);
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
    if (timeoutString != timeoutEnd) config.wifi.disconnectTimeout = timeout;
  }
  MSerial serial;
  serial->print("wifi::timeout: ");
  serial->println(config.wifi.disconnectTimeout);
}

static void configure(WordSplit &args) {
  auto ssid = args.nextWord();
  if (!ssid) {
    MSerial()->print("wifi::configure: missing ssid argument");
    return;
  }
  strncpy(config.wifi.ssid, ssid, sizeof(config.wifi.ssid) - 1);
  if (auto password = args.nextWord()) strncpy(config.wifi.password, password, sizeof(config.wifi.password) - 1);
  else memset(config.wifi.password, 0, sizeof(config.wifi.password));
  MSerial serial;
  serial->print("wifi::configure: set ssid ");
  serial->print(config.wifi.ssid);
  if (strlen(config.wifi.password)) {
    serial->print(" password ");
    serial->print(config.wifi.password);
  }
  serial->println();
}

static void connect(WordSplit &) {
  uint8_t bssid[6];
  int32_t rssi;
  IPAddress ip, gateway, dns1, dns2;
  {
    WifiConnection wifi(config.wifi);
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

namespace tls {

constexpr const uint16_t defaultTlsPort = 443;

static void ping(WordSplit &args) {
  auto address = args.nextWord();
  if (!address) {
    MSerial()->print("tls::ping: failed to parse address\n");
    return;
  }
  if (WifiConnection::ipConnectBroken) {
    IPAddress ip;
    if (ip.fromString(address)) {
      MSerial()->print("tls::ping: tls validation is broken as of firmware version " WIFI_FIRMWARE_LATEST_VERSION
                       " for direct to IP connections\n");
      return;
    }
  }
  auto portString = args.nextWord();
  char *portEnd;
  auto port = strtoul(portString, &portEnd, 10);
  if (portString == portEnd) port = defaultTlsPort;
  if (!port || port > uint16_t(-1)) {
    MSerial()->print("tls::ping: invalid port\n");
    return;
  }
  WifiConnection wifi(config.wifi);
  if (!wifi) {
    MSerial()->print("tls::ping: failed to connect to wifi\n");
    return;
  }
  MSerial()->print("tls::ping: connected to wifi\n");

  {
    WiFiSSLClient client;
    if (!client.connect(address, port)) {
      MSerial()->print("tls::ping: failed to connect to server\n");
      return;
    }
    MSerial()->print("tls::ping: connected to server\n");

    for (auto word = args.nextWord(); word;) {
      if (!client.print(' ') || !client.print(word)) goto sendError;
      if (!(word = args.nextWord())) {
        if (!client.println()) goto sendError;
        break;
      }
      continue;
    sendError:
      MSerial()->print("tls::ping: failed to write all the data\n");
      return;
    }
    MSerial()->print("tls::ping: send complete, waiting for response\n");

    constexpr const size_t maxLen = std::min(255, SERIAL_BUFFER_SIZE - 1);
    uint8_t tlsInput[maxLen];
    // XXX TODO time period is broken, this below counts as 10ms
    constexpr const unsigned int waitingReadInterval = 100;
    /*
      XXX TODO using read() if the sslclient is not connected (FIN/RST received) on
      the esp32s3 causes a fault (probably abort() is called): doing so, it appears
      to print debug data to the Serial input, and it may or may not reset the renesas
      chip, causing undebuggable chaos.

      All the Arduino examples, which work in a single-thread environment, show a busy
      loop read:

      while (client.connected()) {  // sends _SSLCLIENTCONNECTED request and waits
                                    // BUG esp32s3 could receive and handle a connection close here!
        client.read(...);           // sends _SSLCLIENTRECEIVE request and waits
        // parse...
      }

      That means that *almost always* you are catching the closed connection state
      before attempting a read, so no crash occur. However, it is impossible to make
      this safe, because the esp32s3 runs independently of the renesas, and it may
      as well process a connection close event in the middle.
      Under FreeRTOS, this is more evident because the polling loop below sleeps for
      some time after receiving no data (read() == 0).

      For now keep close the connected() and read() call. We cannot even make this a
      critical section, as UART input requires interrupts...

    */
    while (client.connected()) {
      // this is non blocking as the underlying code may return zero (and available() == 0) while still being connected
      auto len = client.read(tlsInput, maxLen);
      if (!len) {
        vTaskDelay(pdMS_TO_TICKS(waitingReadInterval));
        continue;
      }
      MSerial()->write(tlsInput, len);
    }
  }

  MSerial()->print("\ntls::ping: connection closed\n");
}

} // namespace tls

static constexpr const CliCallback callbacks[]{makeCliCallback(version),
                                               makeCliCallback(uptime),
                                               makeCliCallback(debug),
                                               makeCliCallback(echo),
                                               makeCliCallback(scale::mode),
                                               makeCliCallback(scale::tare),
                                               makeCliCallback(scale::calibrate),
                                               makeCliCallback(scale::raw),
                                               makeCliCallback(scale::weight),
                                               makeCliCallback(wifi::status),
                                               makeCliCallback(wifi::configure),
                                               makeCliCallback(wifi::connect),
                                               makeCliCallback(tls::ping),
                                               CliCallback()};

} // namespace cli

} // namespace blastic

void setup() [[noreturn]] {
  using namespace blastic;
  // some gracetime to start `platformio device monitor` after upload or power on
  delay(1000);
  // XXX do not use FreeRTOS functions other than allocating tasks, crashes seen otherwise
  Serial.begin(BLASTIC_MONITOR_SPEED);
  while (!Serial);
  Serial.print("Booting blastic-scale version ");
  Serial.println(version);
  // use 4 KiB of stack, this was seen to trigger a stack overflow in wifi functions
  static cli::SerialCliTask<Serial, 4 * 1024> cli(cli::callbacks);
  static util::StaticTask display(ui::loop, "DisplayTask");
  Serial.print("Starting FreeRTOS scheduler.\n");
  vTaskStartScheduler();
  configASSERT(false && "vTaskStartScheduler() should never return");
}
