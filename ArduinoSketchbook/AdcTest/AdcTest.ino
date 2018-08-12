#include <BookWorm.h>

void setup() {
  BookWorm.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t adc = analogRead(A0);
  uint32_t mv;
  mv = BookWorm.adcToVoltage(adc);
  Serial.printf("adc %u -> %u mv\r\n", adc, mv);
  BookWorm.delayWhileFeeding(1000);
}
