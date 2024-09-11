#ifndef _HEADER_SerialCliTask
#define _HEADER_SerialCliTask

#include <type_traits>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "StaticTask.h"
#include "Mutexed.h"

namespace cli {

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
  using CliFunctionPointer = void (*)(String &params);

  constexpr CliCallback() : cliCommandHash(0), function(nullptr) {}

  constexpr CliCallback(const char *str, CliFunctionPointer function)
      : cliCommandHash(murmur3_32(str)), function(function) {}

  // Reference: https://en.wikipedia.org/wiki/MurmurHash#Algorithm
  constexpr static uint32_t murmur3_32(const char *const str) {
    uint32_t h = 0xfaa7c96c, len = CliCallback::strlen(str), dwordLen = len & ~uint32_t(3),
             leftoverBytes = len & uint32_t(3);
    for (uint32_t i = 0; i < dwordLen; i += 4) {
      uint32_t dword = 0;
      for (uint32_t j = 0; j < 4; j++) dword |= str[i + j] << j * 8;
      h ^= murmur3_32_scramble(dword);
      h = (h << 13) | (h >> 19);
      h = h * 5 + 0xe6546b64;
    }
    uint32_t partialDword = 0;
    for (uint32_t i = 0; i < leftoverBytes; i++) partialDword |= str[dwordLen + i] << i * 8;
    h ^= murmur3_32_scramble(partialDword);
    // XOR with the string length
    h ^= len;
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

  // constexpr strlen
  constexpr static uint32_t strlen(const char *const str) { return str[0] ? 1 + CliCallback::strlen(str + 1) : 0; }

  constexpr static uint32_t murmur3_32_scramble(uint32_t dword) {
    dword *= 0xcc9e2d51;
    dword = (dword << 15) | (dword >> 17);
    dword *= 0x1b873593;
    return dword;
  }

  template <auto &, size_t> friend class SerialCliTask;
};

#define makeCliCallback(func) CliCallback(#func, func)

template <auto &serial, size_t StackSize = configMINIMAL_STACK_SIZE * sizeof(StackType_t)>
class SerialCliTask : public util::StaticTask<StackSize> {

public:
  SerialCliTask(const CliCallback *callbacks, const char *bootMessage = nullptr)
      : util::StaticTask<StackSize>(SerialCliTask::loop, this, "SerialCliTask", configMAX_PRIORITIES - 1),
        callbacks(callbacks) {
    serial.begin(9600);
    while (!serial);
    serial.setTimeout(0);
    // make sure that the static member initializer for the mutex runs
    MSerial p;
    if (bootMessage) p->print(bootMessage);
  }

private:
  using MSerial = util::Mutexed<::Serial>;
  const CliCallback *callbacks;

  static void loop(void *_this) { reinterpret_cast<SerialCliTask *>(_this)->loop(); }
  void loop() {
    static String serialInput;
    static char buff[32];
    while (true) {
      // Read from serial in chunks. This is non blocking as we used setTimeout(0) on initialization
      auto buffRead = serial.readBytes(buff, sizeof(buff) - 1);
      buff[buffRead] = '\0';
      if (!buffRead) {
        vTaskDelay(pdMS_TO_TICKS(250));
        continue;
      }
      // input may contain the null character, sanitize to a newline
      for (char *ch = buff; ch < buff + buffRead; ch++) *ch = *ch ?: '\n';
      serialInput += buff;
      // parse commands line by line
      while (true) {
        auto lineEnd = serialInput.indexOf('\n');
        if (lineEnd == -1) {
          if (serialInput.length() > 512) {
            MSerial()->print(F("Buffer overflow while reading serial input!\n"));
            serialInput = String();
          }
          break;
        }
        size_t commandStart = 0;
        while (isspace(serialInput[commandStart])) commandStart++;
        size_t commandEnd = commandStart;
        while (!isspace(serialInput[commandEnd])) commandEnd++;
        String args = serialInput.substring(commandEnd, lineEnd);
        args.trim();
        serialInput[commandEnd] = '\0';
        auto commandHash = CliCallback::murmur3_32(serialInput.c_str() + commandStart);
        auto callback = callbacks;
        for (; callback->function && callback->cliCommandHash != commandHash; callback++);
        if (callback->function) callback->function(args);
        else MSerial()->print(F("Command not found.\n"));
        serialInput.remove(0, lineEnd + 1);
      }
    }
  }
};

} // namespace cli

#endif
