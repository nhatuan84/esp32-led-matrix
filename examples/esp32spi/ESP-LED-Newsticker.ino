#include <SPI.h>
#include "LedMatrix.h"

#define NUMBER_OF_DEVICES 6
#define CS_PIN 15
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "ssid";
const char* pwd = "pwd";
#endif

HTTPClient http;
int myPage=1;
 
void setup()
{
  Serial.begin(115200);
  delay(4000);   //Delay needed before calling the WiFi.begin
  WiFi.begin(ssid,pwd);
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  if(WiFi.status() != WL_CONNECTED){   //Check WiFi connection status
    Serial.println("Error in WiFi connection");
    return;
  }

  ledMatrix.init();
  ledMatrix.setAlternateDisplayOrientation(); // devices 90° right
  ledMatrix.setIntensity(4); // range is 0-15
}
 
void loop()
{
  String text = "https://newsapi.org/v2/top-headlines?country=de&category=technology&pagesize=10&page=";
  text += myPage;
  text += "&apiKey=xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // put your api-key here
  http.begin(text);
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  int httpCode = http.GET();   //Send the actual POST request

  if(httpCode<0) {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    WiFi.disconnect();
    delay(10000);
    setup();
    myPage=1;
    return;
  }

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(10) +  10*JSON_OBJECT_SIZE(2) + 10*JSON_OBJECT_SIZE(7) + 5023;
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(http.getString());
  http.end();
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    myPage = 1;
    return;
  }

  int totalResults = root["totalResults"].as<int>();
  for (int an = 0; an < 10; an++)  {
  
    text = root["articles"][an]["title"].as<char*>();
    if (!text) {
      myPage = 1;
      return;
    }
    ledMatrix.setText(text);
    int l = ledMatrix.getTextLength() + NUMBER_OF_DEVICES*8;

    for (int i= 0; i < l; i++) {
      ledMatrix.clear();
      ledMatrix.scrollTextLeft();
      ledMatrix.drawText();
      ledMatrix.commit();
      delay(5);
    }
  }
  myPage++;
}

