#include <Arduino.h>
#include <HX711.h>

namespace blastic {

class Scale : public HX711 {
public:
  struct [[gnu::packed]] EEPROMConfig {
    uint8_t dataPin, clockPin;
    float scale;
  };

  Scale(const EEPROMConfig &);
  ~Scale();
};

} // namespace blastic
