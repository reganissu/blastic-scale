#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
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

static constexpr const CliCallback callbacks[]{makeCliCallback(echo), makeCliCallback(weight), CliCallback()};

} // namespace cli

#define stringify(x) #x
#define stringify_value(x) stringify(x)

void setup() {
  delay(5000);
  static cli::SerialCliTask<Serial> cli(
      cli::callbacks,
      String(F("Booted blastic-scale git commit " GIT_COMMIT " worktree " GIT_WORKTREE_STATUS ".\n")).c_str());
  static util::StaticTask display(ui::loop, "DisplayTask");
  vTaskStartScheduler();

  for (;;);
}
