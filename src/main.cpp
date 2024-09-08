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

static void echo(const String &params) {
  auto p = MutexedPrint<Serial>();
  p.print(F("echo: "));
  p.println(params);
}

static void weight(const String &) {
  auto value = blastic::scale.read();
  auto p = MutexedPrint<Serial>();
  p.print(F("weight: "));
  p.println(value);
}

static constexpr const CliCallback callbacks[]{makeCliCallback(echo), makeCliCallback(weight), CliCallback()};

} // namespace cli

void setup() {
  static cli::SerialCliTask<Serial> cli(cli::callbacks);
  static util::StaticTask display(ui::loop, "DisplayTask");
  vTaskStartScheduler();

  for (;;);
}
