#include <Arduino.h>
#include <SoftwareSerial.h>
#include <HX711.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "helper.h"

#define DEBUG
#define DEBUG_ARDUINO_PORT Serial
#define pinDHT 8
#define soilHumidityDataPin A0
#define soilHumidityVccPin 4

#ifdef DEBUG_ARDUINO_PORT
#define DEBUG_MSG(...) DEBUG_ARDUINO_PORT.print( __VA_ARGS__ )
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

String lastCommand = "";

String ESP_status = "unready";
/*void sendCommand(String command, String arg1, String arg2) {
    ESP.println(command+"|"+arg1+"|"+arg2);
    while (!ESP.available())
    {
      DEBUG_MSG_LN("waiting to command response");
    }
    String res = ESP.readString();
    String* resArr = getParsedCommand(res);
    if(resArr[0] == "res")
      DEBUG_MSG_LN(command+" command executed with response: "+ resArr[1]);
}*/

void listenForCommandandResponse() {
  if(ESP.available()) {
    String data = ESP.readStringUntil('\n');
    String* args = getParsedCommand(data);
    DEBUG_MSG_LN(data);
    if(args[0] == "res") {
      if(lastCommand == "getMac") {
        if(args[1] != "unknown" && args[1] != "invalid params") {
          lastCommand = "";
          DEBUG_MSG_LN(lastCommand+" response "+ args[1]);
          macAddress = args[1];
        } else {
           ESP.println("getMac");
        }
      }
    }

    delete args;
  }
}

unsigned long lastMillis = 0;

void setup() {
  delay(5000);
  // put your setup code here, to run once:
  Serial.begin(9600);
  ESP.begin(9600);
  dht.begin();
  ESP.println("getMac");
  lastCommand = "getMac";
}

void loop() {
  listenForCommandandResponse();

  #ifdef DEBUG
  if(Serial.available()) {
    ESP.print(Serial.readString());
  } 
  #endif

  if(millis() - lastMillis >= 0.1*60*1000UL) {

    lastMillis = millis();

    int soilHumidity = getSoilHumidity(soilHumidityVccPin,soilHumidityDataPin);

    DEBUG_MSG("Humidity of soil is:");
    DEBUG_MSG_LN(soilHumidity);

    float t = dht.readTemperature();

    if(isnan(t)) {
      DEBUG_MSG_LN("sensor not found");
    } else {
      DEBUG_MSG_LN("temp is: "+(String)t);
      ESP.println("setTemp|"+macAddress+"|"+((String)t));
    }
  }
  else if(millis() < lastMillis) {
    lastMillis = millis();
  }
}