#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

void spam(void *warmup);

void serial_read(void *warmup);

void print_millis(void *warmup);

void print_micros(void *warmup);

void loop(){}

// unused for now as setting -DconfigCHECK_FOR_STACK_OVERFLOW>0 causes linking issues in linking with PlatformIO
void vApplicationStackOverflowHook(TaskHandle_t *pxTask, signed char *pcTaskName){
  Serial.print(F("! stack overflow by task "));
  Serial.println(reinterpret_cast<const char *>(pcTaskName));
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
