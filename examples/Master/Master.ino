/*
  Modbus-Arduino Example - Master (Modbus IP ESP8266)
  Control a Led on GPIO0 pin using Write Single Coil Modbus Function 

  Current version
  (c)2017 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
*/

#ifdef ESP8266
 #include <ESP8266WiFi.h>
#else
 #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

//Modbus Registers Offsets (0-9999)
const int LED_COIL = 100;
//Used Pins
const int ledPin = 0; //GPIO0

//ModbusIP object
ModbusMasterIP mb;
  
void setup() {
  Serial.begin(74880);
 
  WiFi.begin("your_ssid", "your_password");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mb.begin();

  pinMode(ledPin, OUTPUT);
  mb.addCoil(LED_COIL);
  mb.connect()
  mb.readSlave(COIL(LED_COIL), 1, MB_FC_READ_COILS);
  mb.send();
}
 
void loop() {
   //Call once inside loop() - all magic here
  mb.get();

   //Attach ledPin to LED_COIL register
  digitalWrite(ledPin, mb.Coil(LED_COIL));
}