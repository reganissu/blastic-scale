#include "Scale.h"

namespace blastic {

Scale::Scale(const EEPROMConfig &config) {
  begin(config.dataPin, config.clockPin);
  set_scale(config.scale);
}
Scale::~Scale() { power_down(); }

} // namespace blastic
