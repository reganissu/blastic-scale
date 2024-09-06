#include "SerialCliTask.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

void loop() {}

static void cliPing(const String &params) { Serial.print(F("pong\n")); }

static constexpr const CliCallback cliCallbacks[]{CliCallback("ping", cliPing), CliCallback()};

SerialCliTask cliTask(Serial, cliCallbacks);

void setup() {
  cliTask.create(cliCallbacks);
  vTaskStartScheduler();

  for (;;);
}
