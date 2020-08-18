#define LCD_SCK  0x02 //PA1
#define LCD_CS   0x08 // PA3
#define LCD_MOSI 0x02 // PC1

#define P1 0x04 //D
#define P2 0x08 // D
#define BTN 0x01 // C
enum {
  ST1, ST2, ST3, ST4, NON
};

unsigned char numbers[10][8] = {
  {0x00, 0x38, 0x44, 0x4C, 0x54, 0x64, 0x44, 0x38}, // 0
  {0x00, 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x7c},
  {0x00, 0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x7c},
  {0x00, 0x38, 0x44, 0x04, 0x18, 0x04, 0x44, 0x38},
  {0x00, 0x08, 0x18, 0x28, 0x48, 0x7C, 0x08, 0x08},
  {0x00, 0x7C, 0x40, 0x78, 0x04, 0x04, 0x44, 0x38}, // 5
  {0x00, 0x38, 0x40, 0x40, 0x78, 0x44, 0x44, 0x38},
  {0x00, 0x7C, 0x04, 0x08, 0x10, 0x20, 0x20, 0x20},
  {0x00, 0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x38},
  {0x00, 0x38, 0x44, 0x44, 0x3C, 0x04, 0x44, 0x38}, // 9
};
unsigned char l_block[8][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,1,1,1,1,1,1},
  {0,0,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
};
void setup() {
  DDRA |= LCD_SCK | LCD_CS;
  DDRC |= LCD_MOSI;
  PORTA &= ~(LCD_SCK | LCD_CS);
  PORTC &= ~LCD_MOSI;


  DDRD &= (~P1);
  DDRD &= (~P2);
  DDRC &= ~BTN;

  PORTD |= P1 | P2;
  PORTC |= BTN;

  delayMicroseconds(50);
  lcd_set_instruction(0x30);
  lcd_set_instruction(0x30);
  lcd_set_instruction(0x30);

  lcd_set_instruction(0x30);
  lcd_set_instruction(0x06);
  lcd_set_instruction(0x0C);

  lcd_set_instruction(0x01);
  delay(10);
  lcd_cls();
  delay(1000);
}

char last_btn = 0;
int cnt = 0;
int current_state = NON;
int previous_state = NON;

int y_pos = 0;
int x_pos = 1;
char b_end = 0;
unsigned long p_millis = 0;
unsigned long c_millis = 0;

unsigned long p_millis1 = 0;
unsigned long c_millis1 = 0;

void loop() {
  c_millis = millis();
  c_millis1 = millis();
  if(c_millis1 - p_millis1 > 1000 && !b_end){
    p_millis1 = c_millis1;
    for(int i = 0; i < 8; i++){
      unsigned char c_block = 0;
      for(int j = 0; j < 8; j++){
        if(l_block[i][j] == 1) c_block |= 0x80 >> j;
      }
      
      int draw_x = y_pos + i > 31 ? 9 : 1; 
      int draw_y = y_pos + i > 31 ? y_pos + i - 31 : y_pos + i;
      if(draw_x == 9){
        if(draw_y == 31){
          b_end = 1;
          break;
        }
      }
      
      set_data(draw_x,draw_y, c_block);
    }
    
    y_pos+=2v;
  }
}
void lcd_cls() {
  for (int j = 0; j < 32; j++) {
    for (int i = 0; i < 16; i++) {
      set_data(i, j, 0);
    }
  }
}
void set_data(char x, char y, short data) {
  lcd_set_instruction(0x36);
  lcd_set_instruction(0x80 | (y & 0x7f)); // Y
  lcd_set_instruction(0x80 | (x & 0x0f)); // X

  lcd_set_instruction(0x30);
  //Write 128 bit data
  lcd_set_data((0xff00 & data) >> 8); //0101 0101
  lcd_set_data(0x00ff & data); // 0101 0101

}

void lcd_set_data(char data) {
  PORTA |= LCD_CS; // CS HIGH

  shift_out(0xFA);
  shift_out(data & 0xF0);
  shift_out((data & 0x0F ) << 4);

  PORTA &= ~LCD_CS; // CS LOW
  delayMicroseconds(50);
}

void lcd_set_instruction(char data) {
  PORTA |= LCD_CS; // CS HIGH

  shift_out(0xF8);
  shift_out(data & 0xF0);
  shift_out((data & 0x0F ) << 4);

  PORTA &= ~LCD_CS; // CS LOW

  delayMicroseconds(50);
}
// MSB FIRST
void shift_out(char data) {
  for (int i = 0; i < 8; i++) {
    if (data & (0x80 >> i)) {
      PORTC |= LCD_MOSI;  // DATA HIGH
    } else {
      PORTC &= ~LCD_MOSI; // DATA LOW
    }
    PORTA |= LCD_SCK;
    PORTA &= ~LCD_SCK;
  }

}
