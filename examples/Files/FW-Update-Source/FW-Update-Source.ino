/*
  Modbus Library for Arduino Example - Modbus RTU Firmware Update - ESP8266/ESP32
  Update main node to update from.
  Hosts simple web-server to upload firmware file and writes it to update target.
  
  (c)2021 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*/
#include <ModbusRTU.h>
ModbusRTU rtu;

#if defined(ESP8266)

#else
#include <WebServer.h>
#include <Update.h>
#endif
WebServer web(80);

#if defined(ESP8266)
 #include <SoftwareSerial.h>
 // SoftwareSerial S(D1, D2, false, 256);

 // receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
 // connect RX to D2 (GPIO4, Arduino pin 4), TX to D1 (GPIO5, Arduino pin 4)
 SoftwareSerial S(4, 5);
#endif

#define UPDATE_ENABLE 301
#define UPDATE_FILE 301
#define SLAVE_ID 1
#define BLOCK_SIZE 256

bool updating = false;
Modbus::ResultCode result = Modbus::EX_GENERAL_FAILURE;
bool cb(Modbus::ResultCode event, uint16_t transactionId, void* data) { // Modbus Transaction callback
  if (event != Modbus::EX_SUCCESS)                  // If transaction got an error
    Serial.printf("Modbus result: %02X\n", event);  // Display Modbus error code
  result = event;
  return true;
}

void handlePage() {
  String output = F(R"EOF(
<html><body>\
 <form method='POST' action='/update' enctype='multipart/form-data'>\
  Update firmware:<br>\
  <input type='file' name='update'> <input type='submit' value='Update firmware'>\
 </form>\
</body></html>\
)EOF");
  web.sendHeader("Connection", "close");
  web.sendHeader("Cache-Control", "no-store, must-revalidate");
  web.sendHeader("Access-Control-Allow-Origin", "*");
  web.send(200, "text/html; charset=utf-8", output);
}

void updateHandle() {
  web.sendHeader("Connection", "close");
  web.sendHeader("Refresh", "10; url=/");
  web.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
}

void updateUploadHandle() {
  uint8_t* data = nullptr;
  uint16_t remaining;
  HTTPUpload& upload = web.upload();
  switch (upload.status) {
  case UPLOAD_FILE_START:
    result = Modbus::EX_GENERAL_FAILURE;
    rtu.writeCoil(SLAVE_ID, UPDATE_ENABLE, true, cb);
    while (rtu.server()) {
        rtu.task();
        yield();
    }
    updating = (result == Modbus::EX_SUCCESS);
    Serial.print("O");
  break;
  case UPLOAD_FILE_WRITE:
    if (!updating)
        break;
    Serial.print("o");
    data = upload.buf;
    remaining = upload.currentSize >> 2;
    while (remaining) {
        uint16_t amount = (remaining > BLOCK_SIZE)?256:remaining;
        result = Modbus::EX_GENERAL_FAILURE;
        rtu.writeFileRec(SLAVE_ID, UPDATE_FILE, 0, amount, data, cb);
        while (rtu.server()) {
          rtu.task();
          yield();
        }
        remaining -= amount;
        if (result != Modbus::EX_SUCCESS) {
            updating = false;
            break;
        } 
        Serial.print(".");
    }
  break;
  case UPLOAD_FILE_END:
    if (!updating)
        break;
    rtu.writeCoil(SLAVE_ID, UPDATE_ENABLE, false);
    while (rtu.server()) {
      rtu.task();
      yield();
    }
    updating = false;
    Serial.println("!");
  break;
  default:
      if (updating) {
        rtu.writeCoil(SLAVE_ID, UPDATE_ENABLE, false);
        while (rtu.server()) {
          rtu.task();
          yield();
        }
        updating = false;
      }
      Serial.print("X");
  }
}

void setup() {
    Serial.begin(115200);
#if defined(ESP8266)
    S.begin(9600, SWSERIAL_8N1);
    rtu.begin(&S);
#else
    Serial1.begin(9600, SERIAL_8N1);
    rtu.begin(&Serial1);
    rtu.client();
 #endif
    web.on("/update", HTTP_POST, updateHandle, updateUploadHandle);
    web.on("/", HTTP_GET, handlePage);
}

void loop() {
  web.handleClient();
  rtu.task();
  yield();
}