#include <Arduino.h>
#include <SoftwareSerial.h>
#include <HX711.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "helper.h"

#define DEBUG
#define DEBUG_ARDUINO_PORT Serial
#define RELE_HEATING 7
#define DOUT 6
#define CLK  5
#define RELE_1 11 //čerpadlo voda
#define RELE_2 10 //čerpadlo voda + hnojivo
#define RELE_3 12 //čerpadlo hnojivo
#define RELE_4 13 //test topení, jinak budoucí ventilátor
#define FLOAT_SENSOR_1 9 //mýsící nádoba
#define FLOAT_SENSOR_2 A1 //nádoba na hnojivo
#define FLOAT_SENSOR_3 A2 //nádoba na vodu
#define PINDHT 8
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
#define ESPTX 3 // zelený kabel zapojený na ESP do TX
#define ESPRX 2 // žlutý kabel zapojený na ESP do RX
#else
#define ESPTX 1
#define ESPRX 0
#endif

HX711 scale;
const float calibration_factor = 2300;

DHT dht(PINDHT,DHT22);

SoftwareSerial ESP(ESPTX,ESPRX);

//greenhouse basic vars
String macaddress = "";

String lastCommand = "";

String ESP_status = "unready";

//greenhouse config vars
bool tempConfigDone = false;
int configTemp;
int configTempTolerantN;
int configTempTolerantP;

bool irrigationConfigDone = false;
int configIrrigationWater;
int configIrrigationFert;
int configIrrigationTotal;
String configIrrigationRatio;

bool irrigationSoilHumidityDone = false;
int configIrrigationSoilHumidity;

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

void sendESPCommand(String command) {
  ESP.println(command);

  #ifdef DEBUG
  Serial.println(command);
  #endif

  lastCommand = command;
}

void sendNotification(String command, bool val) {
  ESP.println("notify|"+macaddress+"|"+command+"|"+val);

  #ifdef DEBUG
  Serial.println(command);
  #endif
}

void listenForCommandandResponse() {
  if(ESP.available()) {
    String data = ESP.readStringUntil('\n');
    String* args = getParsedCommand(data);
    DEBUG_MSG_LN(data);
    //commands logic
    if(args[0] == "setIrrigation") {
      configIrrigationFert = args[1].toInt();
      configIrrigationWater = args[2].toInt();
      configIrrigationRatio = args[3];
      configIrrigationTotal = args[4].toInt();
      irrigationConfigDone = true;
      DEBUG_MSG_LN("set irrigation config");
    }

    if(args[0] == "setIrrigationSoilHumidity") {
      configIrrigationSoilHumidity = args[1].toInt();
      irrigationSoilHumidityDone = true;
      DEBUG_MSG_LN("set irrigation soil humidity config");
    }

    if(args[0] == "setTargetTemp") {
      configTemp = args[1].toInt();
      configTempTolerantN = args[2].toInt();
      configTempTolerantP = args[3].toInt();
      tempConfigDone = true;
      DEBUG_MSG_LN("set temperature config");
    }

    //response logic
    if(args[0] == "res") {
      if(lastCommand == "getMac") {
        if(args[1] != "unknown" && args[1] != "invalid params") {
          DEBUG_MSG_LN(lastCommand+" response "+ args[1]);
          lastCommand = "";
          macaddress = args[1];
        } else {
           ESP.println("getMac");
        }
      }

      if(lastCommand == "getStatus") {
        if(args[1] != "unknown" && args[1] != "invalid params") {
          DEBUG_MSG_LN(lastCommand+" response "+ args[1]);
          lastCommand = "";
          ESP_status = args[1];
        } else {
           ESP.println("getStatus");
        }
      }
    }

    delete [] args;
  };
}

double getWeight() {
  scale.set_scale(calibration_factor);
  return scale.get_units();
}

void vyprazdnitHlavniNadobu() {
  int weight;
  do {
  digitalWrite(RELE_2,LOW);
  weight = getWeight();
  DEBUG_MSG_LN(weight);
  } while(weight > 3);
  digitalWrite(RELE_2,HIGH);
}

void zalit() {
  if(analogRead(FLOAT_SENSOR_2) == LOW || analogRead(FLOAT_SENSOR_3) == LOW) {
    DEBUG_MSG_LN("nelze zalít, chybí jedna z tekutin");

    if(analogRead(FLOAT_SENSOR_2) == LOW)
    sendNotification("outOfFertiliser",true);

    if(analogRead(FLOAT_SENSOR_3) == LOW)
    sendNotification("outOfWater",true);


    return;
  }

  DEBUG_MSG_LN("všechny nádoby jsou plné");
  int weight;
  scale.tare();
  delay(100);
  int fertWeight = configIrrigationWater + configIrrigationFert;
  // zalití vodou
  do
  {
    digitalWrite(RELE_1,LOW);
    //DEBUG_MSG_LN("čerpání vody....");
    if(digitalRead(FLOAT_SENSOR_3) == LOW) {
      //DEBUG_MSG_LN(configIrrigationRatio.substring(configIrrigationRatio.indexOf(':')+1,configIrrigationRatio.length()));
      break;
    }

    weight = getWeight();
    //DEBUG_MSG_LN(weight);
    delay(100);
  }while(weight <= configIrrigationWater);
  digitalWrite(RELE_1,HIGH);
  DEBUG_MSG_LN("voda načerpána");

  // zalití hnojivem
  do
  {
    digitalWrite(RELE_3,LOW);
    //DEBUG_MSG_LN("čerpání hnojiva....");
    if(digitalRead(FLOAT_SENSOR_2) == LOW)
    break;
    weight = getWeight();
    //DEBUG_MSG_LN(weight);
    delay(100);
  }while(weight <= fertWeight);
  digitalWrite(RELE_3,HIGH);

  vyprazdnitHlavniNadobu();

  if(analogRead(FLOAT_SENSOR_2) == LOW)
    sendNotification("outOfFertiliser",true);
  else
    sendNotification("outOfFertiliser",false);
  
  

  if(analogRead(FLOAT_SENSOR_3) == LOW)
    sendNotification("outOfWater",true);
  else
    sendNotification("outOfWater",false);
  
  //Serial.println("Došla voda");
}

unsigned long lastCheckMillis = 0;

void repeatCommandIfNoResponse() {
  if(lastCommand != "") {
    if(millis() - lastCheckMillis >= 60*1000UL) {
        lastCheckMillis = millis();
        if(lastCommand != "")
        sendESPCommand(lastCommand);
    } else if(millis() < lastCheckMillis) {
        lastCheckMillis = millis();
    }
  } else {
    lastCheckMillis = millis();
  }
}

unsigned long lastHumidityMillis = 0;
unsigned long lastTempMillis = 0;
bool fireOnce = true;
bool heating = false;
int lastSoilHumidity;
float lastAirHumidity;
float lastTemp;

void updateHumidity() {
  int soilHumidity = getSoilHumidity(soilHumidityVccPin,soilHumidityDataPin);
  lastSoilHumidity = soilHumidity;
  DEBUG_MSG("Humidity of soil is:");
  DEBUG_MSG_LN(soilHumidity);
  ESP.println("setSoilHumidity|"+macaddress+"|"+((String)soilHumidity)); 

  float airHumidity = dht.readHumidity();
  lastAirHumidity = airHumidity;
  DEBUG_MSG("Humidity of air is:");
  DEBUG_MSG_LN(airHumidity);
  ESP.println("setAirHumidity|"+macaddress+"|"+((String)airHumidity));
}

void updateTemp() {
  float temp = dht.readTemperature();

  if(isnan(temp)) {
    DEBUG_MSG_LN("sensor not found");
  } else {
    DEBUG_MSG_LN("temp is: "+(String)temp);
    lastTemp = temp;
    ESP.println("setTemp|"+macaddress+"|"+((String)temp));
  }
}

void heatingAndCooling() {
  if(lastTemp) {
    if(lastTemp < (configTemp+configTempTolerantN)){
       digitalWrite(RELE_HEATING,LOW);
      heating = true;
    }

    if(lastTemp >= configTemp && heating) {
      digitalWrite(RELE_HEATING,HIGH);
      heating = false;
    }

    if(lastTemp > (configTemp+configTempTolerantP))
    {}
  }
}

void mainLoop() {
  if(fireOnce) {
    fireOnce = false;

    updateHumidity();

    if(lastSoilHumidity > configIrrigationSoilHumidity )
    zalit();
  }

  if(millis() - lastHumidityMillis >= 60*60*1000UL) {//60
    lastHumidityMillis = millis();

    updateHumidity();

    if(lastSoilHumidity > configIrrigationSoilHumidity )
    zalit();
    
  }
  else if(millis() < lastHumidityMillis) {
    lastHumidityMillis = millis();
  }

  if(millis() - lastTempMillis >= 1*60*1000UL) {
    lastTempMillis = millis();
    updateTemp();
  }
  else if(millis() < lastTempMillis) {
    lastTempMillis = millis();
  }

  heatingAndCooling();
}

void setup() {
  pinMode(FLOAT_SENSOR_1,INPUT); //mísící nádoba
  pinMode(FLOAT_SENSOR_2,INPUT); //nádoba na hnojivo
  pinMode(FLOAT_SENSOR_3,INPUT); //nádoba na vodu
  pinMode(RELE_1,OUTPUT);
  pinMode(RELE_2,OUTPUT);
  pinMode(RELE_3,OUTPUT);
  pinMode(RELE_4,OUTPUT);
  pinMode(RELE_HEATING, OUTPUT);
  
  digitalWrite(RELE_1,HIGH);
  digitalWrite(RELE_2,HIGH);
  digitalWrite(RELE_3,HIGH);
  digitalWrite(RELE_4,HIGH);
  digitalWrite(RELE_HEATING,HIGH);

  Serial.begin(9600);
  ESP.begin(9600);
  dht.begin();
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  sendESPCommand("getStatus");
}

void loop() {
  listenForCommandandResponse();

  repeatCommandIfNoResponse();

  //waiting to ESP to connect
  if(ESP_status != "ready" && lastCommand != "getStatus")
    sendESPCommand("getStatus");

  //get mac address of ESP
  if(ESP_status == "ready" && macaddress == "" && lastCommand != "getMac") {
    sendESPCommand("getMac");
  }

  #ifdef DEBUG
  if(Serial.available()) {
    ESP.print(Serial.readString());
  } 
  #endif

  //when everything is ready, start greenhouse work
  if(ESP_status == "ready" 
  && macaddress != "" 
  && tempConfigDone
  && irrigationConfigDone 
  && irrigationSoilHumidityDone) {
    mainLoop();
  }
}