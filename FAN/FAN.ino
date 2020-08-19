#define FAN 0x10 // PB 4

void setup() {
  DDRB |= FAN;
}

void loop() {
  PORTB |= FAN;
  delay(5000);
  PORTB &= ~FAN;
  delay(5000);
}
