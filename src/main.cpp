#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "SerialCliTask.h"

void loop() {}

namespace cli {

static void ping(const String &params) {
  auto p = MutexedPrint<Serial>();
  p.print(F("pong, args: "));
  p.println(params);
}

static constexpr const CliCallback callbacks[]{makeCliCallback(ping), CliCallback()};

} // namespace cli

void setup() {
  static cli::SerialCliTask<Serial> task(cli::callbacks);
  vTaskStartScheduler();

  for (;;);
}
