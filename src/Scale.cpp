#include <algorithm>
#include "blastic.h"
#include "Scale.h"

namespace blastic {

namespace scale {

static StaticSemaphore_t mutexBuffer;
static SemaphoreHandle_t mutex = xSemaphoreCreateMutexStatic(&mutexBuffer);

int32_t raw(const EEPROMConfig &config, size_t medianWidth, TickType_t timeout) {
  configASSERT(medianWidth);
  auto start = xTaskGetTickCount();
  if (!xSemaphoreTake(mutex, timeout)) return invalidRead;
  const auto sck = config.clockPin, dt = config.dataPin;
  // poweroff the controller and release the mutex
  auto release = [sck]() {
    digitalWrite(sck, HIGH);
    delayMicroseconds(64);
    configASSERT(xSemaphoreGive(mutex));
  };
  pinMode(sck, OUTPUT);
  pinMode(dt, INPUT);
  // power cycle the controller
  digitalWrite(sck, HIGH);
  delayMicroseconds(64);
  digitalWrite(sck, LOW);

  int32_t reads[medianWidth];
  // read #medianWidth values. Read and discard one extra value first if we need to set the chan A gain mode to 64
  for (int i = config.chanAMode == gain_t::GAIN_64 ? -1 : 0; i < medianWidth; i++) {
    // wait for data ready
    while (digitalRead(dt) == HIGH) {
      if (timeout == portMAX_DELAY || xTaskGetTickCount() - start < timeout) {
        vTaskDelay(pdMS_TO_TICKS(minReadDelayMillis));
        continue;
      }
      // timed out
      release();
      if (debug) {
        MSerial serial;
        serial->print(F("scale: timed out waiting for data, median index "));
        serial->println(i);
      }
      return invalidRead;
    }
    delayMicroseconds(1); // HX711 datasheet T1
    // drive sck pin to receive data from dt pin
    taskENTER_CRITICAL();
    int32_t value = 0;
    for (auto i = config.chanAMode == gain_t::GAIN_128 ? 25 : 27, mask = 0x800000; i; i--, mask >>= 1) {
      digitalWrite(sck, HIGH);
      delayMicroseconds(1); // HX711 datasheet T3
      if (digitalRead(dt) == HIGH) value |= mask;
      digitalWrite(sck, LOW);
      delayMicroseconds(1); // HX711 datasheet T4
    }
    taskEXIT_CRITICAL();
    if (i < 0) continue;
    // sign extend
    if (value & 0x800000) value |= 0xff000000;
    reads[i] = value;
  }
  release();
  std::sort(reads, reads + medianWidth);
  if (medianWidth % 2) return reads[medianWidth / 2];
  return (reads[medianWidth / 2 - 1] + reads[medianWidth / 2]) / 2;
}

} // namespace scale

} // namespace blastic
