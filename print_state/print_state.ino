#define P1 0x04 //D
#define P2 0x08 // D
#define BTN 0x01 // C
enum {
  ST1, ST2, ST3, ST4, NON
};

//0 0 -> 1 0 d
void setup() {
  Serial.begin(9600);
  DDRD &= (~P1);
  DDRD &= (~P2);
  DDRC &= ~BTN;

  PORTD |= P1 | P2;
  PORTC |= BTN;
}
char last_btn = 0;
int cnt = 0;
int current_state = NON;
int previous_state = NON;


unsigned long p_millis = 0;
unsigned long c_millis = 0;

void loop() {
  c_millis = millis();
  if (c_millis - p_millis > 1) {
    p_millis = c_millis;
    //Read Pin
    int A = (PIND & P1) > 0;
    int B = (PIND & P2) > 0;
    char btn = (PINC & BTN) > 0;
    // Record State
    if (A == 0 && B == 1) current_state = ST1;
    else if (A == 0 && B == 0) current_state = ST2;
    else if (A == 1 && B == 0) current_state = ST3;
    else if (A == 1 && B == 1) current_state =  ST4;

    //State check
    if (previous_state == ST2 && current_state == ST3) {
      cnt++;
      Serial.println(cnt);
    }
    else if (previous_state == ST2 && current_state == ST1) {
      cnt--;
      Serial.println(cnt);
    }
    if (btn != last_btn) {
      Serial.println(btn == 0 ? "DOWN!" : "UP!");
    }
    //Save Previous States
    previous_state = current_state;
    last_btn = btn;
  }
}
