#ifndef _HEADER_Mutexed
#define _HEADER_Mutexed

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace util {

template <auto &obj> class Mutexed {
  inline static StaticSemaphore_t buffer;
  inline static SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutexStatic(&buffer);

public:
  Mutexed() { configASSERT(xSemaphoreTakeRecursive(Mutexed::mutex, portMAX_DELAY)); }
  Mutexed(const Mutexed &) = delete;
  Mutexed &operator=(const Mutexed &) = delete;
  ~Mutexed() { configASSERT(xSemaphoreGiveRecursive(Mutexed::mutex)); }

  auto operator->() const { return &obj; }
  decltype(obj) operator*() const { return obj; }
};

} // namespace util

#endif
