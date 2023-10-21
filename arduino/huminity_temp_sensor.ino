#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

const int LED_PIN = 16;
const int BUTTON_PIN = 15;
bool DEFAULT_LED_STATE = false;
int btnState;
int lastBtnState = HIGH;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

int appendFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return -1;
  }
  if(file.print(message)){
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
    return -2;
  }
  file.close();
  return 0;
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

int deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
    return -1;
  }
  return 0;
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

const char* RECORD_PATH = "/record.csv";
const char* LOG_PATH = "/debug.log";

void log(char* str){
  Serial.print(str);
  appendFile(SD, LOG_PATH, str);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, !DEFAULT_LED_STATE);

  if (! sht31.begin(0x44)) {   
    log("Check circuit. SHT31 not found\n");
    while (1) delay(1);
  }

  if(!SD.begin(5)){
    log("Card Mount Failed\n");
    while (1) delay(1);
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    log("No SD card attached\n");
    while (1) delay(1);
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    log("MMC\n");
  } else if(cardType == CARD_SD){
    log("SDSC\n");
  } else if(cardType == CARD_SDHC){
    log("SDHC\n");
  } else {
    log("UNKNOWN\n");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // listDir(SD, "/", 0);
  // createDir(SD, "/mydir");
  // listDir(SD, "/", 0);
  // removeDir(SD, "/mydir");
  // listDir(SD, "/", 2);
  // writeFile(SD, "/hello.txt", "Hello ");
  // appendFile(SD, "/hello.txt", "World!\n");
  // readFile(SD, "/hello.txt");
  // deleteFile(SD, "/foo.txt");
  // renameFile(SD, "/hello.txt", "/foo.txt");
  // readFile(SD, "/foo.txt");
  // testFileIO(SD, "/test.txt");
  // Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  // Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  
  //if LED always on, something with initial phase
  digitalWrite(LED_PIN, DEFAULT_LED_STATE);
}

int cnt = 0;
int SAMPLE_CYCLE = 10;

void loop() {
  int error_flag = 0;
  // put your main code here, to run repeatedly:
  int currentBtnState = digitalRead(BUTTON_PIN);
  if(lastBtnState == HIGH && currentBtnState == LOW) {
    log("The btn state changed from HIGH to LOW\n");
    digitalWrite(LED_PIN, !DEFAULT_LED_STATE);
    //clean record
    error_flag = deleteFile(SD, RECORD_PATH);
    delay(1000);
  }

  lastBtnState = currentBtnState;
  delay(1000);
  if (cnt >= SAMPLE_CYCLE) {
    cnt = 0;
    float temp = sht31.readTemperature();
    float hum = sht31.readHumidity();

    if (!isnan(temp)) {
      char temp_result[8];
      dtostrf(temp, 5, 2, temp_result);
      log(temp_result); 
      log("\t\t");

      error_flag |= appendFile(SD, RECORD_PATH, temp_result);
    } else { 
      log("Failed to read temperature\n");
      error_flag |= -1;
    }
  
    if (!isnan(hum)) {
      char hum_result[8];
      dtostrf(hum, 5, 2, hum_result);
      log(hum_result);
      log("\n");
      error_flag |= appendFile(SD, RECORD_PATH, ",");
      error_flag |= appendFile(SD, RECORD_PATH, hum_result);
    } else { 
      log("Failed to read humidity\n");
      error_flag |= -1;
    }
    error_flag |= appendFile(SD, RECORD_PATH, "\n");
  }else{
    cnt += 1;
  }
  if (error_flag != 0) {
    digitalWrite(LED_PIN, !DEFAULT_LED_STATE);
  } else if(currentBtnState == HIGH) {
    digitalWrite(LED_PIN, DEFAULT_LED_STATE);
  }

}
