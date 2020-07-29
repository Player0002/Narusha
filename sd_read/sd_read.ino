#include <SPI.h>
#include <SD.h>
#define SD_PIN 14
#define MAX 100
char* arr[MAX] = {};
int rear = 0, front = 0;

int isEmpty(){
  return rear == front;
}
int isFull(){
  return (rear + 1) % MAX == front;
}
void enqueue(char* data){
  if(!isFull()){
    rear = (rear +1)%MAX;
    arr[rear] = data;
  }
}
char* dequeue(){
  if(!isEmpty()){
    front = (front + 1) % MAX;
    return arr[front];
  }
}

File myFile;

char* buf = NULL;
int idx = 0;
char requirePass = 0;
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

void loop() {
  if(myFile){
    // read from the file until there's nothing else in it:
    if (myFile.available()) {
      char data = myFile.read();
      if(data == ';') requirePass = 1;
      if(!requirePass){
        buf[idx++] = data;
        if(data == '\n'){
          buf[idx++] = 0;
          enqueue(buf);
          Serial.println("ENQUEUE");
          Serial.println(buf);
          idx = 0;
          buf = (char*)malloc(sizeof(char) * 256);
        }
      }else{
        if(data == '\n') requirePass = 0;
      }
    }else{
      myFile.close();
      while(!isEmpty()){
        char* data = dequeue();
        Serial.println(data);
        free(data);
      }
    }
    // close the file:
  }
}
