#include <Arduino.h>
#include "helper.h"

//======================================================================
// HELPING FUNCTIONS
//======================================================================
//https://forum.arduino.cc/index.php?topic=41389.0 credit
char* subStr (char* str, char *delim, int index) {
  char *act, *sub, *ptr;
  static char copy[50];
  int i;

  // Since strtok consumes the first arg, make a copy
  strcpy(copy, str);

  for (i = 1, act = copy; i <= index; i++, act = NULL) {
     //Serial.print(".");
     sub = strtok_r(act, delim, &ptr);
     if (sub == NULL) break;
  }
  return sub;

}

 String getArguments(String data,  int index) {
  char separator = '|';
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String* getParsedCommand(String data) {
    int count = 1;
    char separator = '|';

    data.replace("\r","");
    data.replace("\n","");

    for (size_t i = 0; i < (data.length()-1); i++)
    {
      if(data.charAt(i) == separator)
      count++;
    }

    String* arr = new String[count];

    /*Vector<String>* ret = new Vector<String>(20);
    ret->PushBack("");*/
    
    if(count > 1) {
      for (size_t i = 0; i < count; i++)
      {
        arr[i] = getArguments(data,i);
      }
    } else {
      arr[0] = data;
    }

    return arr;
};

int getSoilHumidity(int vccPin, int dataPin) {
  digitalWrite(vccPin,HIGH);
  delay(100);
  int analog = analogRead(dataPin);
  digitalWrite(vccPin,LOW);
  return analog;
};
//=======================================================================