#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "blastic.h"
#include "SerialCliTask.h"
#include "Scale.h"
#include "Display.h"

using MSerial = util::Mutexed<::Serial>;

namespace blastic {

static constexpr const char PROGMEM version[] = {GIT_COMMIT " worktree " GIT_WORKTREE_STATUS};
static constexpr const EEPROMConfig defaultConfig{.scale = {.dataPin = 2, .clockPin = 3, .scale = 1.0}};

Scale scale(defaultConfig.scale);

} // namespace blastic

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
  const auto arg1 = args.nextWord();
  MSerial serial;
  // TODO all PROGMEM functionality has been disabled with macros, strcmp_P -> strcmp ...
  if (arg1 && strcmp_P(arg1, PSTR("debug"))) modem.debug(*serial, 2);
  auto status = WiFi.status();
  auto wifiVersion = WiFi.firmwareVersion();
  modem.noDebug();
  serial->print(F("wifi::status: status "));
  serial->print(status);
  serial->print(F(" version "));
  serial->println(wifiVersion);
}

} // namespace wifi

static constexpr const CliCallback callbacks[]{makeCliCallback(version), makeCliCallback(echo),
                                               makeCliCallback(scale::weight), makeCliCallback(wifi::status),
                                               CliCallback()};

} // namespace cli

void setup() {
  // some gracetime to start `platformio device monitor` after upload or power on
  delay(1000);
  {
    MSerial serial;
    serial->begin(9600);
    serial->print(F("Booting blastic-scale version "));
    serial->println(F(blastic::version));
    // use 1 KiB of stack, this was seen to trigger a stack overflow in wifi functions
    static cli::SerialCliTask<Serial, 1024> cli(cli::callbacks);
    static util::StaticTask display(ui::loop, "DisplayTask");
    serial->print(F("Starting FreeRTOS scheduler.\n"));
  }
  vTaskStartScheduler();

  for (;;);
}
