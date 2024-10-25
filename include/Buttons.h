#pragma once

#include <array>
#include <functional>
#include "StaticTask.h"
#include <R4_Touch.h>

namespace blastic {

class DebouncedTouchSensor : public TouchSensor {
public:
  using TouchSensor::TouchSensor;
  struct [[gnu::packed]] EEPROMConfig {
    uint8_t pin;
    uint16_t threshold;
    ctsu_pin_settings_t settings;
  };

  bool updateRead();
  operator bool() const { return triggered; }

  static void measurementCallback();

protected:
  volatile uint32_t lastMeasures = 0;
  volatile bool triggered = false;
};

namespace buttons {

constexpr const size_t n = 4;

extern DebouncedTouchSensor sensors[n];

using EEPROMConfig = std::array<DebouncedTouchSensor::EEPROMConfig, n>;

void reset(const EEPROMConfig &config);
void edgeCallback(size_t i, bool rising);

} // namespace buttons

} // namespace blastic
