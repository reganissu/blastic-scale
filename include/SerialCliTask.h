#ifndef _HEADER_SerialCliTask
#define _HEADER_SerialCliTask

#include <algorithm>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "Mutexed.h"
#include "StaticTask.h"

namespace blastic {

namespace cli {

class WordSplit;

/*
  A CliCallback struct contains the MurMur3 hash of the command string, and
  the corresponding function pointer to call.

  The constructors are all constexpr, so that the compiler can avoid emitting
  the string for the command.
*/

class CliCallback {

public:
  using CliFunctionPointer = void (*)(WordSplit &args);

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

/*
  Use makeCliCallback(ns::func) as CliCallback("ns::func", ns::func)
*/
#define makeCliCallback(func) CliCallback(#func, func)

/*
  Utility class to tokenize a string by spaces.
  Each token returned by nextWord() is a pointer to the buffer
  passed in the constructor, trimmed left and right of spaces.
  The buffer is modified in-place with '\0' terminators, so
  it is not possible to reuse WordSplit on the same pointer.
*/

class WordSplit {
public:
  char *str;

  WordSplit(char *str) : str(str) {}

  char *nextWord() {
    while (*str && isspace(*str)) str++;
    if (!*str) return nullptr;
    auto result = str++;
    while (*str && !isspace(*str)) str++;
    if (*str) *str++ = '\0';
    return result;
  }

  bool nextWordIs(const char *str) {
    auto word = nextWord();
    return word && !strcmp(word, str);
  }
};

/*
  Simple implementation of a command line interface over Serial.

  The SerialCliTask implements a FreeRTOS task that reads from serial and
  parses command in the form:

    <command> [args...]

  The list of commands is provided by the user as an array of CliCallback structures,
  matching command strings to function pointers. This array can be created as a
  constexpr expression, to avoid emitting the command strings in the binary:

  static constexpr const CliCallback callbacks[]{
    CliCallback("func1", ::func1),
    CliCallback("func2", tools::func2),
    CliCallback()
  };

  The terminating empty CliCallback() is necessary.

  This CLI task runs with the highest priority available, in order to be able to work
  even when other tasks hang for any reason.

  This class is a template in order to use util::Mutexed<serial>.
*/

template <auto &serial, size_t StackSize = configMINIMAL_STACK_SIZE * sizeof(StackType_t)>
class SerialCliTask : public util::StaticTask<StackSize> {

public:
  // task delay in poll loop, milliseconds
  static constexpr const int32_t pollInterval = 250;

  // Note: the serial must be initialized alreay
  SerialCliTask(const CliCallback *callbacks)
      : util::StaticTask<StackSize>(SerialCliTask::loop, this, "SerialCliTask", configMAX_PRIORITIES - 1),
        callbacks(callbacks) {
    /*
      Read operations busy poll using millis(). This task
      runs with the maximum priority, so we must not starve the other
      tasks in blocking read operations. Polling with task delays is
      handled in loop().
    */
    MSerial()->setTimeout(0);
  }

private:
  const CliCallback *callbacks;

  static void loop(void *_this) { reinterpret_cast<SerialCliTask *>(_this)->loop(); }
  void loop() {
    /*
      Avoid String usage at all costs because it uses realloc(),
      shoot in your foot with pointer arithmetic.

      This function reads serial input into a static buffer, and
      parses a command line per '\n'-terminated line of input.

      Command names are trimmed by WordSplit.
    */
    constexpr const size_t maxLen = std::min(255, SERIAL_BUFFER_SIZE - 1);
    char serialInput[maxLen + 1];
    static size_t len = 0;
    // polling loop
    while (true) {
      auto oldLen = len;
      // this is non blocking as we used setTimeout(0) on initialization
      len += serial.readBytes(serialInput + len, maxLen - len);
      if (oldLen == len) {
        vTaskDelay(pdMS_TO_TICKS(pollInterval));
        continue;
      }
      // input may contain the null character, sanitize to a newline
      for (auto c = serialInput + oldLen; c < serialInput + len; c++) *c = *c ?: '\n';
      // make sure this is always a valid C string
      serialInput[len] = '\0';
      // parse loop
      while (true) {
        auto lineEnd = strchr(serialInput, '\n');
        if (!lineEnd) {
          if (len == maxLen) {
            MSerial()->print(F("Buffer overflow while reading serial input!\n"));
            // discard buffer
            len = 0;
          }
          break;
        }
        // truncate the command line C string at '\n'
        *lineEnd = '\0';
        WordSplit commandLine(serialInput);
        auto command = commandLine.nextWord();
        uint32_t commandHash;
        // skip if line is empty
        if (!command || !*command) goto shiftLeftBuffer;
        commandHash = CliCallback::murmur3_32(command);
        for (auto callback = callbacks; callback->function; callback++)
          if (callback->cliCommandHash == commandHash) {
            callback->function(commandLine);
            goto shiftLeftBuffer;
          }
        {
          MSerial mserial;
          mserial->print(F("Command "));
          mserial->print(command);
          mserial->print(F(" not found.\n"));
        }
        // move bytes after newline to start of buffer, repeat
      shiftLeftBuffer:
        auto nextLine = lineEnd + 1;
        auto leftoverLen = len - (nextLine - serialInput);
        memmove(serialInput, nextLine, leftoverLen);
        len = leftoverLen;
      }
    }
  }
};

} // namespace cli

} // namespace blastic

#endif
