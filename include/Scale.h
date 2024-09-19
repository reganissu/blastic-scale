#ifndef _HEADER_Scale
#define _HEADER_Scale

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace blastic {

namespace scale {

// HX711Mode can be cast to an integer and used as index in arrays below
enum class HX711Mode : uint8_t { A128 = 0, B = 1, A64 = 2 };
constexpr const char *HX711ModeStrings[] = {"A128", "B", "A64"};

struct [[gnu::packed]] EEPROMConfig {
  uint8_t dataPin, clockPin;
  HX711Mode mode;
  struct [[gnu::packed]] calibration {
    int32_t tareRawRead, weightRawRead;
    float weight;
  } calibrations[3];
};

constexpr const int32_t invalidRead = 0x800000;
constexpr const float invalidWeight = NAN;
constexpr const uint32_t minReadDelayMillis = 1000 / 80;

int32_t raw(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY);

inline float weight(const EEPROMConfig &config, size_t medianWidth = 1, TickType_t timeout = portMAX_DELAY) {
  auto &calibration = config.calibrations[uint8_t(config.mode)];
  if (calibration.weightRawRead - calibration.tareRawRead == 0) return invalidWeight;
  auto value = raw(config, medianWidth, timeout);
  if (value == invalidRead) return invalidWeight;
  return calibration.weight * float(value - calibration.tareRawRead) /
         float(calibration.weightRawRead - calibration.tareRawRead);
}

} // namespace scale

} // namespace blastic

#endif
