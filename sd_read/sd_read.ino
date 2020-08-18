
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


double arr[MAX][4] = {};
int rear = 0, front = 0;

int isEmpty() {
  return rear == front;
}
int isFull() {
  return (rear + 1) % MAX == front;
}
void enqueue(double x, double y, double xspd, double yspd) {
  if (!isFull()) {
    rear = (rear + 1) % MAX;
    arr[rear][0] = x;
    arr[rear][1] = y;
    arr[rear][2] = xspd;
    arr[rear][3] = yspd;
  }
}
int dequeue() {
  if (!isEmpty()) {
    front = (front + 1) % MAX;
    return front;
  }
}

File myFile;
enum {
  data_G, data_X, data_Y, data_Z, data_E, data_F
};
char requirePass = 0;
int startRecording = 0;
int currentInstruction = 0;

double x = 0;
double y = 0;
double z = 0;
double e = 0;
int f = 0;

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
  while (!Serial);
  // wait for serial port to connect. Needed for native USB port only

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  // open the file for reading:
  myFile = SD.open("test.txt");
  Serial.println("test.txt:");


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
int cnt = 0;

unsigned long c_millis_1 = 0;
unsigned long c_millis_2 = 0;
unsigned long p_millis_1 = 0;
unsigned long p_millis_2 = 0;
void loop() {
  if (!isEmpty() && TIMSK1 == 0 && TIMSK2 == 0) {
    int data = dequeue();
    double x = arr[data][0];
    double y = arr[data][1];
    int xspd = (int) arr[data][2];
    int yspd = (int)arr[data][3];
    goXLocation(x, xspd);
    goYLocation(y, yspd);
    Serial.println(x);
  }
  if (myFile) {
    c_millis_1 = millis();
    c_millis_2 = millis();
    // read from the file until there's nothing else in it:
    if (myFile.available()) {
      if (c_millis_1 - p_millis_1 > 10) { // 10 Millis to read
        p_millis_1 = c_millis_1;
        if (!isFull()) {
          char data = myFile.read();
          if (data == 13) data = myFile.read();
          if (data == ';') requirePass = 1;
          if (!requirePass) {
            if (data == 'G') { //Start recording
              startRecording = data_G;
            } else if (data == 'X') {
              startRecording = data_X;
            } else if (data == 'Y') {
              startRecording = data_Y;
            } else if (data == 'Z') {
              startRecording = data_Z;
            } else if (data == 'E') {
              startRecording = data_E;
            } else if (data == 'F') {
              startRecording = data_F;
            }

            if (startRecording == data_G) { //Read data GNN
              int gTemp = 0;
              while ((data = myFile.read()) != ' ') {
                gTemp += data - '0';
                gTemp *= 10;
              }
              gTemp /= 10;
              currentInstruction = gTemp;
            }

            //X, Y, Z, E Data Insert
            if (startRecording == data_X || startRecording == data_Y || startRecording == data_Z || startRecording == data_E) {
              int front_num = 0;
              double sub_num = 0;
              double sub_size = 1;
              int complete = 0;
              while ((data = myFile.read()) != ' ' && data != '\n' && data != -1) {
                if (data == 13) continue;
                if (data == '.') {
                  complete = 1;
                  continue;
                }
                if (complete == 0) {
                  front_num += data - '0';
                  front_num *= 10;
                } else {
                  sub_num += data - '0';
                  sub_num *= 10;
                  sub_size *= 10;
                }
              }
              front_num /= 10;
              sub_num /= 10;
              if (startRecording == data_X) {
                x = front_num + (sub_num / sub_size);
              }
              if (startRecording == data_Y) {
                y = front_num + (sub_num / sub_size);
              }
              if (startRecording == data_Z) {
                z = front_num + (sub_num / sub_size);
              }
              if (startRecording == data_E) {
                e = front_num + (sub_num / sub_size);
              }
            }
            // Read F Value
            if (startRecording == data_F) {
              int value = 0;
              while ((data = myFile.read()) != '\n' && data != -1) {
                if (data == 13) continue;
                value += data - '0';
                value *= 10;
              }
              value /= 10;
              f = value;
            }

            //Save datas
            if (data == '\n' || data == -1) {

              int xspeed = 800;
              int yspeed = 800;

              float length_x = abs(p_x - x);
              float length_y = abs(p_y - y);

              if (length_x != 0 && length_y != 0)
              {
                if (length_x > length_y)
                {
                  xspeed = 800 * (length_y / length_x);
                  if (xspeed < 200) {
                    xspeed = 800;
                    yspeed = 800 * (length_x / length_y);
                  }
                }
                else if (length_x < length_y)
                {
                  yspeed = 800 * (length_x / length_y);
                  if (yspeed < 200) {
                    yspeed = 800;
                    xspeed = 800 * (length_y / length_x);
                  }
                }
              }

              enqueue(x, y, xspeed, yspeed);
              startRecording = 0;
              p_x = x;
              p_y = y;
            }
          } else {
            if (data == '\n') {
              requirePass = 0;
            }
          }
        }
      }

    } else {
      myFile.close();
      Serial.println("End process");
    }
    // close the file:
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
