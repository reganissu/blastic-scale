#pragma once

#include "blastic.h"
#include "StaticTask.h"
#include "Looper.h"

namespace blastic {

class Submitter {

public:
  enum class Action : uint32_t { NONE = 0, OK = 1, NEXT = 2, BACK = 3 };

  struct [[gnu::packed]] EEPROMConfig {
    float threshold;
  };

  Submitter(const char *name, UBaseType_t priority);
  void action(Action action);

protected:
  util::Looper<1024> painter;
  util::StaticTask<4 * 1024> task;

  Action idling();
  virtual void loop() [[noreturn]];
  static void loop(void *_this) [[noreturn]] { reinterpret_cast<Submitter *>(_this)->loop(); }
};

} // namespace blastic
