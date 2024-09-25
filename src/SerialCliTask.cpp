#include "SerialCliTask.h"

namespace cli {

namespace details {

// task delay in poll loop, milliseconds
static constexpr const uint32_t pollInterval = 250;

void loop(const SerialCliTaskState &_this, Stream &input, util::MutexedGenerator<Print> outputMutexGen) {
  /*
    Avoid String usage at all costs because it uses realloc(),
    shoot in your foot with pointer arithmetic.

    This function reads input into a static buffer, and
    parses a command line per '\n'-terminated line of input.

    Command names are trimmed by WordSplit.
  */
  constexpr const size_t maxLen = std::min(255, SERIAL_BUFFER_SIZE - 1);
  char serialInput[maxLen + 1];
  size_t len = 0;
  // polling loop
  while (true) {
    auto oldLen = len;
    // this is non blocking as we used setTimeout(0) on initialization
    len += input.readBytes(serialInput + len, maxLen - len);
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
          outputMutexGen.lock()->print("cli: buffer overflow while reading input\n");
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
      commandHash = util::murmur3_32(command);
      for (auto callback = _this.callbacks; callback->function; callback++)
        if (callback->cliCommandHash == commandHash) {
          callback->function(commandLine);
          goto shiftLeftBuffer;
        }
      {
        auto output = outputMutexGen.lock();
        output->print("cli: command not found :");
        output->println(command);
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

} // namespace details

} // namespace cli
