
//For sd card
#include <SPI.h>
#include <SD.h>
#define SD_PIN 14
#define MAX 100

//For 3d Printer
#define X_DIR 0x20     // C
#define X_STEP 0x80    //D
#define X_STOP 0x04    // C
#define XYEENABLE 0x40 // D

#define Y_DIR 0x80  // DDRC 0x80
#define Y_STEP 0x40 //DDRC 0x40
#define Y_STOP 0x08 // DDRC 0x08

#define ONE_MM 80
#define ONE_CM 800

#define X_LEFT PORTC | X_DIR
#define X_RIGHT PORTC & ~(X_DIR)

#define Y_UP PORTC | Y_DIR
#define Y_DOWN PORTC & ~(Y_DIR)

#define BASESPEED 800

#define MAX 100
#define DEBUG 0

#define EOF -1
File fp;
int p_x = 0, p_y = 0;

enum
{
  x_left,
  x_right,
  y_up,
  y_down
};

volatile char is_x_reset = 0;
int x_distance = 0;

volatile char is_y_reset = 0;
int y_distance = 0;

float off_y = 0;
float off_x = 0;

float ANGLE(int x)
{
  return (PI * x) / 180.0;
}

void y_move(int y_dis, int DIR, int speed)
{
  if (DIR == y_up)
    PORTC = Y_UP;
  if (DIR == y_down)
    PORTC = Y_DOWN;
  y_distance = y_dis;
  TIMSK3 = 0x00;
  OCR3A = speed;
  TIMSK3 = 0x02;
}
void x_move(int x_dis, int DIR, int speed)
{
  //----------------------------- Dir change
  if (DIR == x_left)
    PORTC = X_LEFT;
  if (DIR == x_right)
    PORTC = X_RIGHT;
  //----------------------------- Distance change
  x_distance = x_dis;
  TIMSK1 = 0x00;
  OCR1A = speed;
  //----------------------------- Enable Timer X
  TIMSK1 = 0x02;
}
void reset()
{
  is_x_reset = 1;
  is_y_reset = 1;
  x_move(32000, x_left, BASESPEED);
  y_move(32000, y_up, BASESPEED);
  while (is_y_reset != 0 || is_x_reset != 0)
    ;
}

int px = 0, py = 0;

void goXLocation(float x, int speed)
{
  int dir = x_right;
  if (px - x > 0)
    dir = x_left;
  else if (px - x < 0) dir = x_right;
  else if (px - x == 0) return;
  float distance = abs(px - x);
  distance /= 0.0125;

  int overed = (int)(distance + 0.5);
  off_x += overed - distance;
  if (off_x > 0)
  {
    int sum = (int)off_x;
    off_x -= sum;
    overed -= sum;
  }
  else if (off_x < 0)
  {
    int sum = abs((int)off_x);
    off_x += sum;
    overed += sum;
  }
  px = x;
  x_move(overed, dir, speed);
}
void goYLocation(float y, int speed)
{
  int dir = y_down;
  if (py - y > 0)
    dir = y_up;
  else if (py - y == 0) return;
  float amount = abs(py - y);
  float distance = amount;
  distance /= 0.0125;
  int overed = (int)(distance + 0.5);
  off_y += overed - distance;
  if (off_y > 0)
  {
    int sum = (int)off_y;
    off_y -= sum;
    overed -= sum;
  }
  else if (off_y < 0)
  {
    int sum = abs((int)off_y);
    off_y += sum;
    overed += sum;
  }
  py = y;
  y_move(overed, dir, speed);
}


void setup() {
  Serial.begin(9600);
  if(!SD.begin(SD_PIN)){
    Serial.println("INIT FAILED");
    while(1);
  }
  fp = SD.open("test.txt");


  // Serial.begin(9600);
  DDRC |= Y_DIR | Y_STEP;
  DDRC &= ~Y_STOP;
  DDRC |= X_DIR; // RIGHT DIRECTION
  DDRD |= X_STEP | XYEENABLE;
  DDRC &= ~(X_STOP);

  // TIMSK0 = 0x00; // Disable DELAY

  //X LOC
  TCCR1A = 0x00;
  TCCR1B = 0X0a;
  TCCR1C = 0x00;
  OCR1A = 800;
  TIMSK1 = 0x00;
  //Y Loc
  TCCR3A = 0x00;
  TCCR3B = 0X0a;
  TCCR3C = 0x00;
  OCR3A = 800;
  TIMSK3 = 0x00;

}



//------------------------------------------ Str Queue
int str_rear;
int str_front;

char str_queue[MAX];

int str_is_full() {
    return ((str_rear +1)%MAX) == str_front;
}
int str_is_empty() {
    return str_rear == str_front;
}
int str_enqueue(char c) {
    if (str_is_full()) {
        return -1;
    }
    str_rear = (str_rear + 1) % MAX;
    str_queue[str_rear] = c;
    return 1;
}

char str_dequeue() {
    if (str_is_empty()) {
        return -1;
    }
    str_front = (str_front + 1) % MAX;
    return str_queue[str_front];
}
//---------------------------------------------- Pos Queue
typedef struct Pos { // for save position
    double x;
    double y;
    int x_speed;
    int y_speed;
}Pos;

int pos_rear;
int pos_front;

Pos pos_queue[MAX];

int pos_is_full() {
    return ((pos_rear + 1) % MAX) == pos_front;
}
int pos_is_empty() {
    return pos_rear == pos_front;
}
int pos_enqueue(struct Pos pos) {
    if (pos_is_full()) {
        return -1;
    }
    pos_rear = (pos_rear + 1) % MAX;
    pos_queue[pos_rear].x = pos.x;
    pos_queue[pos_rear].y = pos.y;
    pos_queue[pos_rear].x_speed = pos.x_speed;
    pos_queue[pos_rear].y_speed = pos.y_speed;
    return 1;
}
Pos pos_dequeue() {
    if (pos_is_empty()) {
        return Pos{ -1,-1,-1, -1 };
    }
    pos_front = (pos_front + 1) % MAX;
    return pos_queue[pos_front];
}

unsigned long index = 0;

char is_ended = 0;

void fileManagement() {
    if (fp.available()) fp.close();

    fp = SD.open("test.txt");
    
    fp.seek(index); // set position to first

    char c = fp.read();
    index = fp.position(); // Save current position
    if (c == EOF) fp.close(); // END

    int enqueue = str_enqueue(c);
    //If Failed to enqueue ( Queue is full )
    if (enqueue == -1) {
        index--;
        fp.close();
        return;
    }

    fp.close(); // Close File
}

double previous_x = 0;
double previous_y = 0;


double result_x = 0; // Save data
double result_y = 0; 

int result_speed = 0;

int default_speed = 0;

int num_front = 0;
int num_backward = 0;

char required_backward = 0;

char current_inst = 0;

char inst_num = 0;

char repeat_enqueue = 0;
char required_skip = 0;

int* calculate_speed(double px, double py, double x, double y);
int is_inst(char value);
void outData() {
    char c = str_dequeue();

    if (c!= '\n' && required_skip) return;

    if (c == ';') {
        required_skip = 1;
        return;
    }
    // Save decoded data
    if (c == '\n' || repeat_enqueue == 1) { // end of line
        
        if (required_skip == 1) {
            required_skip = 0;
            return;
        }
        
        int* speeds = calculate_speed(previous_x, previous_y, result_x, result_y);
        Pos pos{ result_x, result_y, speeds[0], speeds[1] };
        free(speeds);
        int enqueue = pos_enqueue(pos);
        if (enqueue == -1) {
            repeat_enqueue = 1;
            return;
        }
        //printf("X : %.3f Y : %.3f x_Speed : %d y_Speed : %d\n", pos.x, pos.y, pos.x_speed, pos.y_speed);
        previous_x = result_x;
        previous_y = result_y;
        repeat_enqueue = 0;
        result_x = 0;
        result_y = 0;

         num_front = 0;
         num_backward = 0;
        return;
    }
    //If Queue is full
    if (c == -1) {
        if (is_ended == 0) {
            is_ended = 1;
            return;
        }
    }

    if (c == ' ') { // end instruction
        required_backward = 0;

        //save x y 
        if (current_inst == 'X') {
            num_front /= 10;
            num_backward /= 10;
            result_x = num_front + (num_backward / 1000.0);
            num_front = 0;
            num_backward = 0;
        }
        else if (current_inst == 'Y') {
            num_front /= 10;
            num_backward /= 10;
            result_y = num_front + (num_backward / 1000.0);
            num_front = 0;
            num_backward = 0;
        }
        else if (current_inst == 'G') {
            inst_num /= 10;
        }
        else if (current_inst == 'F') {
            result_speed /= 10;
        }
        //reset inst
        current_inst = 0;
    }

    //check is instruction
    if (is_inst(c)) {
        current_inst = c;
        return;
    }


    if (current_inst == 'G') {
        inst_num += c - '0';
        inst_num *= 10;
        return;
    }
    else if (current_inst == 'X' || current_inst == 'Y' || current_inst == 'E') {
        if (c == '.') {
            required_backward = 1;
            return;
        }
        if (!required_backward) {
            num_front += c - '0';
            num_front *= 10;
        }
        else {
            num_backward += c - '0';
            num_backward *= 10;
        }
    }
    else if (current_inst == 'F') {
        result_speed += c - '0';
        result_speed *= 10;
    }
    

    
}
int is_inst(char value) {
    return value == 'G' || value == 'X' || value == 'Y' || value =='E' || value =='F';
}
int* calculate_speed(double px, double py, double x, double y) { // return value like {200, 500}
    double length_x = abs(px - x);
    double length_y = abs(py - y);

    int x_speed = BASESPEED; // Base speed;
    int y_speed = BASESPEED;

    if (length_x != 0 && length_y != 0) {
        if (length_x > length_y)
        {
            x_speed = BASESPEED * (length_y / length_x);
            if (x_speed < 200) {
                x_speed = BASESPEED;
                y_speed = BASESPEED * (length_x / length_y);
            }
        }
        else if (length_x < length_y)
        {
            y_speed = BASESPEED * (length_x / length_y);
            if (y_speed < 200) {
                y_speed = BASESPEED;
                x_speed = BASESPEED * (length_y / length_x);
            }
        }
    }
    int* speeds = (int*)malloc(sizeof(int) * 2);
    speeds[0] = (int)(x_speed + 0.5); speeds[1] = (int)(y_speed + 0.5);
    return speeds;
}

void dequeue_data() {
  if(TIMSK1 != 0 || TIMSK3 != 0) return;
  Pos pos = pos_dequeue();
  if (pos.x == -1 && pos.y == -1) {
      return;
  }
  Serial.print("Parsed Data : ");
  Serial.print(pos.x);
  Serial.print(", ");
  Serial.print(pos.y);
  Serial.print(" | ");
  Serial.print(pos.x_speed);
  Serial.print(", ");
  Serial.println(pos.y_speed);
  goXLocation(pos.x, pos.x_speed);
  goYLocation(pos.y, pos.y_speed);
}



unsigned long c_millis_1 = 0;
unsigned long c_millis_2 = 0;
unsigned long c_millis_3 = 0;
unsigned long p_millis_1 = 0;
unsigned long p_millis_2 = 0;
unsigned long p_millis_3 = 0;


void loop() {
  c_millis_1 = millis();
  c_millis_2 = millis();
  c_millis_3 = millis();
  if(c_millis_1 - p_millis_1 > 10){
    p_millis_1 = c_millis_1;
    fileManagement();
  }
  if(c_millis_2 - p_millis_2 > 10){
    p_millis_2 = c_millis_2;
    outData();
  }
  if(c_millis_3 - p_millis_3 > 1000){
    p_millis_3 = c_millis_3;
    dequeue_data();
  }
}

volatile int x_step_count = 0;
volatile char x_step_toggle = 0;
ISR(TIMER1_COMPA_vect)
{
  if (x_step_toggle == 0)
  {
    x_step_toggle = 1;
    PORTD |= X_STEP;
  }
  else
  {
    x_step_toggle = 0;
    PORTD &= ~(X_STEP);
    x_step_count++;
    char x_limit_switch = PINC & X_STOP;
    if (x_step_count >= x_distance)
    {
      x_step_count = 0;
      TIMSK1 = 0x00;
    }
    if (x_limit_switch)
    {
      x_step_count = 0;
      TIMSK1 = 0;
    }
  }
}

volatile int y_step_count = 0;
volatile char y_step_toggle = 0;
ISR(TIMER3_COMPA_vect)
{
  if (y_step_toggle == 0)
  {
    y_step_toggle = 1;
    PORTC |= Y_STEP;
  }
  else
  {
    y_step_toggle = 0;
    PORTC &= ~(Y_STEP);
    y_step_count++;
    char y_limit_switch = PINC & Y_STOP;
    if (y_step_count >= y_distance)
    {
      y_step_count = 0;
      TIMSK3 = 0x00;
    }
    if (y_limit_switch)
    {
      y_step_count = 0;
      TIMSK3 = 0;
    }
  }
}
