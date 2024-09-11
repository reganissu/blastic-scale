#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "blastic.h"
#include "SerialCliTask.h"
#include "Scale.h"
#include "Display.h"

namespace blastic {

static constexpr const EEPROMConfig defaultConfig{.scale = {.dataPin = 2, .clockPin = 3, .scale = 1.0}};

Scale scale(defaultConfig.scale);

} // namespace blastic

namespace cli {

using MSerial = util::Mutexed<::Serial>;

static void echo(const String &params) {
  MSerial p;
  p->print(F("echo: "));
  p->println(params);
}

static void weight(const String &) {
  auto value = blastic::scale.read();
  MSerial p;
  p->print(F("weight: "));
  p->println(value);
}

static void wifi(const String &args) {
  MSerial p;
  if(args == F("debug")) modem.debug(*p, 2);
  auto status = WiFi.status();
  auto version = WiFi.firmwareVersion();
  modem.noDebug();
  p->print(F("wifi status:"));
  p->print(status);
  p->print(F(" version:"));
  p->println(version);
}

static constexpr const CliCallback callbacks[]{makeCliCallback(echo), makeCliCallback(weight), makeCliCallback(wifi),
                                               CliCallback()};

} // namespace cli

void setup() {
  // some gracetime to start `platformio device monitor` after upload or power on
  delay(1000);
  static cli::SerialCliTask<Serial, 1024> cli(
      cli::callbacks,
      String(F("Booted blastic-scale git commit " GIT_COMMIT " worktree " GIT_WORKTREE_STATUS ".\n")).c_str());
  static util::StaticTask display(ui::loop, "DisplayTask");
  vTaskStartScheduler();

  for (;;);
}
