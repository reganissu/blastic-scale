#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "SerialCliTask.h"
#include "Scale.h"

namespace blastic {

struct [[gnu::packed]] EEPROMConfig {
  Scale::EEPROMConfig scale;
};

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
  static cli::SerialCliTask<Serial> task(cli::callbacks);
  vTaskStartScheduler();

  for (;;);
}
