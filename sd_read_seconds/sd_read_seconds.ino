#include <SPI.h>
#include <SD.h>
#define SD_PIN 14
#define MAX 100
char arr[MAX] = {};
int rear = 0, front = 0;
int cntn = 0;
int eofs = 0;
int isEmpty() {
  return rear == front;
}
int isFull() {
  for (int i = 0; i < 20; i++) {
    if ((rear + i + 1) % MAX == front) return 1;
  }
  return 0;
}
void enqueue(char* data, int sizes) {
  int i = 0;
  if (sizes < 20) eofs++;
  while (i < sizes) {
    if (!isFull()) {
      rear = (rear + 1) % MAX;
      if (data[i] == '\n') cntn++;
      arr[rear] = data[i++];
    }
  }
}
char dequeue() {
  front = (front + 1) % MAX;
  if (arr[front] == '\n') cntn--;
  return arr[front];
}
File myFile;
char* buf = NULL;
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
  buf = (char*)malloc(sizeof(char) * 21);
}
unsigned long c_millis_1 = 0;
unsigned long c_millis_2 = 0;
unsigned long p_millis_1 = 0;
unsigned long p_millis_2 = 0;

void loop() {
  if (myFile) {
    c_millis_1 = millis();
    c_millis_2 = millis();
    if (myFile.available()) {
      if (c_millis_1 - p_millis_1 > 1) {
        p_millis_1 = c_millis_1;
        if (!isFull()) {
          int bytes = myFile.read(buf, 20);
          if (bytes < 20) {
            buf[bytes] = '\n';
            enqueue(buf, bytes + 1);
          } else {
            enqueue(buf, bytes);
          }
          if (bytes == 20) {
            buf = (char*)malloc(sizeof(char) * 21);
          }
        }
      }
      if (c_millis_2 - p_millis_2 > 1) {
        p_millis_2 = c_millis_2;
        if (cntn > 0 || eofs) {
          if (!isEmpty()) {
            char data;
            while ((data = dequeue()) != '\n') Serial.print(data);
            Serial.println();
          }
        }
      }
    }
  }
}
