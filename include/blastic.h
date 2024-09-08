#ifndef _HEADER_blastic
#define _HEADER_blastic

#include "Scale.h"

namespace blastic {

struct [[gnu::packed]] EEPROMConfig {
  Scale::EEPROMConfig scale;
};

extern Scale scale;

} // namespace blastic

#endif
