#include "StaticTask.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

/*
  Simple implementation of a command line interface over Serial.

  The SerialCliTask implements a FreeRTOS task that reads from serial and
  parses command in the form:
    <command> [args...]
  The list of commands is provided by the user as an array of CliCallback structures,
  matching command strings to function pointers.

  This CLI task runs with the highest priority available, in order to be able to work
  even when other tasks hang for any reason.
*/

class CliCallback {

  /*
    A CliCallback struct contains the MurMur3 hash of the command string, and
    the corresponding function pointer to call.

    The constructors are all constexpr, so that the compiler can avoid emitting
    the string for the command.
  */

public:
  using CliFunctionPointer = void (*)(const String &params);

  constexpr CliCallback() : cliCommandHash(0), function(nullptr) {}

  constexpr CliCallback(const char *str, CliFunctionPointer function)
      : cliCommandHash(murmur3_32(str)), function(function) {}

  // Reference: https://en.wikipedia.org/wiki/MurmurHash#Algorithm
  constexpr static uint32_t murmur3_32(const char *str) {
    auto strStart = str;
    uint32_t h = 0xfaa7c96c, dword = 0;
    while (murmur3_32_dwordFromStr(str, dword)) {
      h ^= murmur3_32_scramble(dword);
      h = (h << 13) | (h >> 19);
      h = h * 5 + 0xe6546b64;
    }
    // process leftover bytes (this is a no-op when there is none)
    h ^= murmur3_32_scramble(dword);
    // XOR with the string length
    h ^= strStart - str;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
  }

private:
  const uint32_t cliCommandHash;
  const CliFunctionPointer function;

  constexpr static uint32_t murmur3_32_dwordFromStr(const char *&str, uint32_t &dword) {
    dword = 0;
    for (int i = 0; i < 4; i++) {
      if (!str[i]) return false;
      dword |= *str++ << i * 8;
    }
    return true;
  }

  constexpr static uint32_t murmur3_32_scramble(uint32_t dword) {
    dword *= 0xcc9e2d51;
    dword = (dword << 15) | (dword >> 17);
    dword *= 0x1b873593;
    return dword;
  }

  friend void SerialCliLoop(typeof(Serial) &serial, const CliCallback *callbacks);
};

template <size_t StackSize = configMINIMAL_STACK_SIZE * sizeof(StackType_t)> class SerialCliTask {

public:
  StaticTask<StackSize> task;

  SerialCliTask(typeof(Serial) &serial, const CliCallback *callbacks) : serial(serial), callbacks(callbacks) {}

  /*
    Setup the task.
    Argument is an array of CliCallback structures, terminated by one dummy entry (callback.func == nullptr).
  */
  TaskHandle_t create(const CliCallback *callbacks) {
    Serial.begin(9600);
    while (!Serial);
    serial.setTimeout(0);
    this->callbacks = callbacks;
    TaskHandle_t handle = task.create(SerialCliTask::loop, this, "SerialCliTask", configMAX_PRIORITIES - 1);
    if (!handle) serial.print(F("Failed to create serial CLI task!\n"));
    return handle;
  }

protected:
  typeof(Serial) &serial;
  const CliCallback *callbacks;

  static void loop(void *_this) {
    auto &_thisRef = *reinterpret_cast<SerialCliTask *>(_this);
    SerialCliLoop(_thisRef.serial, _thisRef.callbacks);
  }
};

void SerialCliLoop(typeof(Serial) &serial, const CliCallback *callbacks);
