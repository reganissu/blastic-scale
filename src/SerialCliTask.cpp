#include "SerialCliTask.h"

void SerialCliLoop(typeof(Serial) &serial, const CliCallback *callbacks) {
  String serialInput;
  while (true) {
    // read from serial in chunks
    char buff[32];
    // this is non blocking as we used setTimeout(0) on init
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
          serial.print(F("Buffer overflow while reading serial input!\n"));
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
      else serial.print(F("Command not found.\n"));
      serialInput.remove(0, lineEnd + 1);
    }
  }
}
