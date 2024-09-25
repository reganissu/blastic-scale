#pragma once

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "AnnotatedFloat.h"

namespace blastic {

namespace scale {

// HX711Mode can be cast to an integer and used as index in arrays below
enum class HX711Mode : uint8_t { A128 = 0, B = 1, A64 = 2 };

struct [[gnu::packed]] EEPROMConfig {
  uint8_t dataPin, clockPin;
  HX711Mode mode;
  struct [[gnu::packed]] calibration {
    int32_t tareRawRead, weightRawRead;
    float weight;
    operator bool() const { return this->weightRawRead != this->tareRawRead; }
  } calibrations[3];
  auto &getCalibration() { return calibrations[uint8_t(mode)]; }
  auto &getCalibration() const { return calibrations[uint8_t(mode)]; }
};

constexpr const int32_t readErr = 0x800000;
constexpr const util::AnnotatedFloat weightCal = util::AnnotatedFloat("cal"), weightErr = util::AnnotatedFloat("err");
constexpr const uint32_t minReadDelayMillis = 1000 / 80;  // max output rate is 80Hz

/*
  Read a raw value from HX711. Can run multiple measurements and get the median.

  This function switches on and back off the controller. The execution is
  also protected by a global mutex.
*/
int32_t raw(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY);

/*
  As above, but return a computed weight using calibration data.
*/
util::AnnotatedFloat weight(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY);

} // namespace scale

} // namespace blastic
