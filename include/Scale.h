#ifndef _HEADER_Scale
#define _HEADER_Scale

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace blastic {

namespace scale {

enum class gain_t : uint8_t { GAIN_128, GAIN_64 };

struct [[gnu::packed]] EEPROMConfig {
  uint8_t dataPin, clockPin;
  gain_t chanAMode;
  struct [[gnu::packed]] calibration {
    int32_t tareRawRead, weightRawRead;
    float weight;
  } calibrationChanA128, calibrationChanA64;
};

constexpr const int32_t invalidRead = 0x800000;
constexpr const float invalidWeight = NAN;
constexpr const uint32_t minReadDelayMillis = 1000 / 80;

int32_t raw(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY);

inline float weight(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY) {
  auto &calib = config.chanAMode == gain_t::GAIN_64 ? config.calibrationChanA64 : config.calibrationChanA128;
  if (calib.weightRawRead - calib.tareRawRead == 0)
    return invalidWeight;
  auto value = raw(config, medianWidth, timeout);
  if (value == invalidRead)
    return invalidWeight;
  return calib.weight * float(value - calib.tareRawRead) / float(calib.weightRawRead - calib.tareRawRead);
}

} // namespace scale

} // namespace blastic

#endif
