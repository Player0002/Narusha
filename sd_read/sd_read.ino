#include <SPI.h>
#include <SD.h>
#define SD_PIN 14
#define MAX 100
double arr[MAX][2] = {};
int rear = 0, front = 0;

int isEmpty() {
  return rear == front;
}
int isFull() {
  return (rear + 1) % MAX == front;
}
void enqueue(double x, double y) {
  if (!isFull()) {
    rear = (rear + 1) % MAX;
    arr[rear][0] = x;
    arr[rear][1] = y;
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
char* buf = NULL;
int idx = 0;
char requirePass = 0;
int startRecording = 0;
int currentInstruction = 0;

double x = 0;
double y = 0;
double z = 0;
double e = 0;
int f = 0;
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
  buf = (char*)malloc(sizeof(char) * 256);
}
int cnt = 0;

unsigned long c_millis_1 = 0;
unsigned long c_millis_2 = 0;
unsigned long p_millis_1 = 0;
unsigned long p_millis_2 = 0;
void loop() {
  if (myFile) {
    c_millis_1 = millis();
    c_millis_2 = millis();
    // read from the file until there's nothing else in it:
    if (myFile.available()) {
      if (c_millis_1 - p_millis_1 > 1) {
        p_millis_1 = c_millis_1;
        if (!isFull()) {
          char data = myFile.read();
          if (data == 13) data = myFile.read();
          if (data == ';') requirePass = 1;
          if (!requirePass) {
            buf[idx++] = data;
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
            //After Read G Instruction

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
              buf[idx++] = 0;
              enqueue(x,y);

              //            Serial.println("----------------------");
              //            Serial.println(currentInstruction);
              //            Serial.println(x, 3);
              //            Serial.println(y, 3);
              //            Serial.println(z, 3);
              //            Serial.println(e, 4);
              //            Serial.println(f);
              startRecording = 0;
              idx = 0;
              //            Serial.println("END ONE");
            }
          } else {
            if (data == '\n') {
              Serial.println("OK PASS");
              requirePass = 0;
            }
          }
        }
      }
      if (c_millis_2 - p_millis_2 > 1) {
        p_millis_2 = c_millis_2;
        if (!isEmpty()) {
          int data = dequeue();
          Serial.print(arr[data][0]);
          Serial.print(" ");
          Serial.println(arr[data][1]);
        }
      }
    } else {
      myFile.close();
      Serial.println("End process");
    }
    // close the file:
  }

}
