#define X_DIR 0x20  //24 // DDRC 0x20
#define X_STOP 0x04 // 21 // DDRC 0x04
#define X_STEP 0x80 // 16 // DDRD 0x80
#define XYEENABLE 0x40 // 15 //DDRD 0x40

#define Y_DIR 0x80 // DDRC 0x80
#define Y_STEP 0x40 //DDRC 0x40
#define Y_STOP 0x08 // DDRC 0x08

#define Z_DIR 0x04 // DDRB 0x04
#define Z_STEP 0x08 // DDRB 0x08
#define Z_STOP 0x10  // DDRC 0x10

#define ONE_CM 800


volatile char x_flag = 0;
volatile char y_flag = 0;

int y_cm = ONE_CM * 3;
int x_cm = ONE_CM * 6;

char x_reset = 0;
char y_reset = 0;


void setup() {
  Serial.begin(9600);
  //DIR, STEP, ENABLE OUTPUT SETTINGS
  DDRC = X_DIR | Y_DIR | Y_STEP;
  DDRD = X_STEP | XYEENABLE;
  DDRB = Z_DIR | Z_STEP;

  //STOP PIN CLEAR
  DDRC &= ~(X_STOP | Y_STOP | Z_STOP);

  //PULLUP Settings
  PORTC = PORTC | (X_STOP | Y_STOP | Z_STOP);

  // Timer 1 Settings
  TCCR1A = 0x00;
  TCCR1B = 0x0A;
  TCCR1C = 0x00;
  OCR1A = 400;
  TIMSK1 = 0x02;

  // Timer 3 Settings
  TCCR3A = 0x00;
  TCCR3B = 0x0A;
  TCCR3C = 0x00;
  OCR3A = 400;
  TIMSK3 = 0x02;


  //X Settings
  PORTC |= X_DIR; // Default value = 1;
  PORTD &= ~(PORTD & X_STEP);
  //Y Settings
  PORTC |= Y_DIR; // Default value = 1;
  PORTC &= ~(PORTC & Y_STEP);

  //--------------------------------------------
}

#define X_LEFT PORTC | X_DIR
#define X_RIGHT PORTC & ~(PORTC & X_DIR)

#define Y_UP PORTC | Y_DIR
#define Y_DOWN PORTC & ~(PORTC & Y_DIR)

enum{
  x_left, x_right, y_up, y_down
};

void x_move(int x_distance, int x_speed, int x_dir){
  if(x_dir == x_left) PORTC = X_LEFT;
  if(x_dir == x_right) PORTC = X_RIGHT;
  OCR1A = x_speed;
  x_cm = x_distance;
  x_flag = 1;
}
void y_move(int y_distance, int y_speed, int y_dir){
  if(y_dir == y_up) PORTC = Y_UP;
  if(y_dir == y_down) PORTC = Y_DOWN;
  OCR3A = y_speed;
  y_cm = y_distance;
  y_flag = 1;
}

int cnt = 0;
void loop() {
  if (cnt == 0) {
    PORTC &= ~(PORTC & Y_DIR);
    PORTC &= ~(PORTC & X_DIR);
    OCR3A = 400;
    OCR1A = 400;
    x_flag = 1;
    while (x_flag == 1 || y_flag == 1);
    y_flag = 1;
    while (x_flag == 1 || y_flag == 1);
    PORTC |= X_DIR | Y_DIR;
    OCR1A = 200;
    x_flag = 1;
    y_flag = 1;
    cnt++;
  }
}

char x_step_toggle = 0;
int x_limit_switch = 0;
unsigned int x_step_count = 0;
ISR(TIMER1_COMPA_vect) {
  if (x_flag) {
    if (x_step_toggle == 0) {
      x_step_toggle = 1;
      PORTD = PORTD | X_STEP;
    } else {
      x_step_toggle = 0;
      PORTD &= ~(PORTD & X_STEP);
      x_step_count ++;
      
      if (x_step_count >= x_cm) { // Move 4 Cm
        x_step_count = 0;
        x_flag = 0;
      }
      x_limit_switch = PINC & X_STOP;
      if (x_limit_switch == 4) {
        x_step_count = 0;
        x_flag = 0;
      }
    }
  }
}

unsigned int y_step_count = 0; // AVR 에선 2BYTE
char y_step_toggle = 0;
char y_limit_switch = 0;
ISR(TIMER3_COMPA_vect) {
  if (y_flag) {
    if (y_step_toggle == 0)
    {
      y_step_toggle = 1;
      PORTC |= Y_STEP;
    } else {
      y_step_toggle = 0;
      PORTC &= ~(PORTC & Y_STEP);

      y_step_count++; // High -> Low 전체가 한 스텝
      if ((y_step_count >= y_cm || y_step_count >= 15000)) {
        y_step_count = 0;
        y_flag = 0;
      }

      y_limit_switch = PINC & Y_STOP;
      if (y_limit_switch == 8) {
        y_step_count = 0;
        y_flag = 0;
      }
    }
  }
}