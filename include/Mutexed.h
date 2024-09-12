#ifndef _HEADER_Mutexed
#define _HEADER_Mutexed

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace util {

/*
  Utility class to automatically create a mutex for any global object
  that comes from the platform framework, and implements cooperative
  locking for it. Use as this, for example with Serial:

  { // mutex is acquired here
    util::Mutexed<Serial> lockedSerial;
    lockedSerial->print("This write");
    lockedSerial->print(" won't be");
    lockedSerial->print(" interleaved with");
    lockedSerial->println(" other tasks' writes!");
  } // mutex is released here

  // note that direct access to the Serial object here cannot be prevented
  Serial->println("You may see pieces of this string interleaved with other tasks' output!");


  The class uses one static member per template parameter obj. It should be
  initialized in a single thread context (such as in void setup()) by
  acquiring it once.
*/

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
