#include <Arduino.h>
#include <SoftwareSerial.h>
#include <HX711.h>
#include <DHT.h>
#include "helper.h"

#define DEBUG
#define DEBUG_ARDUINO_PORT Serial
#define pinDHT 8

#ifdef DEBUG_ARDUINO_PORT
#define DEBUG_MSG(...) DEBUG_ARDUINO_PORT.printf( __VA_ARGS__ )
#define DEBUG_MSG_LN(...) DEBUG_ARDUINO_PORT.println( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#define DEBUG_MSG_LN(...)
#endif

#ifdef DEBUG
#define ESPTX 3
#define ESPRX 2
#else
#define ESPTX 1
#define ESPRX 0
#endif

DHT dht(pinDHT,DHT22);

SoftwareSerial ESP(ESPTX,ESPRX);

String macAddress;

void setup() {
  delay(6000);
  // put your setup code here, to run once:
  Serial.begin(9600);
  ESP.begin(9600);
  dht.begin();
  if(!(macAddress.length() > 0)) {
    ESP.println("getMac");
    while(!ESP.available()) {
      DEBUG_MSG_LN("waiting to response");
    }
    String res = ESP.readString();
    String* resArray = getParsedCommand(res);

    if(resArray[0]=="res") {
      macAddress = resArray[1];
    }
    delete resArray;
  }
}

void loop() {
  if(Serial.available()) {
    ESP.println(Serial.readString());
  } 

  if(ESP.available()) {
    Serial.print(ESP.readString());
  }

  float t = dht.readTemperature();

  if(isnan(t)) {
    Serial.println("sensor not found");
  } else {
    ESP.println("setTemp|"+macAddress+"|"+(String)t);
  }
  delay(2000);
}