#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

void spam(void *warmup);

void serial_read(void *warmup);

void print_millis(void *warmup);

void print_micros(void *warmup);

void loop(){}

/*
  FreeRTOS guide says "here is no real way to recover from a stack overflow
  when it occurs", so just spam the serial with the error in an endless loop.
*/
void vApplicationStackOverflowHook(TaskHandle_t, char *pcTaskName ){
  for(;;){
    Serial.print(F("! stack overflow by task "));
    Serial.println(reinterpret_cast<const char *>(pcTaskName));
    for(auto start_pause = millis(); millis() - start_pause < 1000;);
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("boot!");

  randomSeed(analogRead(0));

  xTaskCreate(spam, "spam", configMINIMAL_STACK_SIZE, reinterpret_cast<void*>(random(20)), tskIDLE_PRIORITY + 1, nullptr);
  xTaskCreate(serial_read, "serial_read", configMINIMAL_STACK_SIZE, reinterpret_cast<void*>(random(20)), tskIDLE_PRIORITY + 1, nullptr);
  xTaskCreate(print_millis, "print_millis", configMINIMAL_STACK_SIZE, reinterpret_cast<void*>(random(20)), tskIDLE_PRIORITY + 1, nullptr);
  xTaskCreate(print_micros, "print_micros ", configMINIMAL_STACK_SIZE, reinterpret_cast<void*>(random(20)), tskIDLE_PRIORITY + 1, nullptr);

  vTaskStartScheduler();
  for(;;);
}
