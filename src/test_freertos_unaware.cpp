#include <Arduino.h>

void spam(void *warmup){
  auto seconds = reinterpret_cast<long>(warmup);
  delay(seconds * 1000ul);
  Serial.print("start spam ");
  Serial.println(seconds);
  for(;;){
    Serial.println("busy loop");
    auto start_busy_loop = millis();
    for(auto start_busy_loop = millis(); millis() - start_busy_loop < 10000ul;);
    Serial.println("delay");
    delay(seconds * 1000ul);
  }
}

void serial_read(void *warmup){
  auto seconds = reinterpret_cast<long>(warmup);
  delay(seconds * 1000ul);
  Serial.print("start serial_read ");
  Serial.println(seconds);
  while(Serial) Serial.read();
  while(true) delay(1000u);  // must never return
}

void print_millis(void *warmup){
  auto seconds = reinterpret_cast<long>(warmup);
  delay(seconds * 1000ul);
  Serial.print("start millis ");
  Serial.println(seconds);
  for(;;){
    Serial.print("millis ");
    Serial.println(millis());
    delay(10000);
  }
}

void print_micros(void *warmup){
  auto seconds = reinterpret_cast<long>(warmup);
  delay(seconds * 1000ul);
  Serial.print("start micros ");
  Serial.println(seconds);
  for(;;){
    Serial.print("micros ");
    Serial.println(micros() / 1000ul);
    delay(10000);
  }
}
