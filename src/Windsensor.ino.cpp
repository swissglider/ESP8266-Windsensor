# 1 "/var/folders/gm/v95wn_cs2n9dl0vlrjc9kx_w0000gn/T/tmpTgSYpC"
#include <Arduino.h>
# 1 "/Users/diener/MEGAsync/Node.js.Project/ESP8266-Windsensor/src/Windsensor.ino"



#include <ESP8266WiFi.h>


#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>


#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>




#include <ESP8266HTTPClient.h>

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <ArduinoJson.h>




const char* apName = "esp8266_Windsensor";
const char* hostName = "esp8266_Windsensor";
const char* passwort = "8266";





#define SERIAL_BAUD 115200
String device = "ArduinoSensor";
String channel = "12";
String name = "Windsensor_Storen";


String webSockerURI1 = "http://192.168.86.167:1880/ArduinoSensorWS";
HTTPClient http;


#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);


const int windPin = D4;
const float windFactor = 1;
const int measureTime = 5;
volatile unsigned int windCounter = 0;
float windSpeed = 0.0;
unsigned long timeL = 0;
void setup();
void loop();
void writeWebSocketMessage(String json, String sockerURI);
void readWindSensor(String& json);
void countWind();
void show_values();
#line 59 "/Users/diener/MEGAsync/Node.js.Project/ESP8266-Windsensor/src/Windsensor.ino"
void setup() {
    Serial.begin(115200);
    Serial.println("Booting");




    WiFiManager wifiManager;


    wifiManager.autoConnect(apName, passwort);


    Serial.println("connected...yeey :)");




    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(hostName);

    ArduinoOTA.setPassword(passwort);
      ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";


    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());





  Serial.begin(SERIAL_BAUD);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  timeL = millis();

}

void loop() {



  ArduinoOTA.handle();





  String wind_json = "";
  readWindSensor(wind_json);
  writeWebSocketMessage(wind_json, webSockerURI1);
  show_values();
}




void writeWebSocketMessage(String json, String sockerURI){
  http.begin(sockerURI);
  int httpCode = http.POST(json);
  http.end();
}




void readWindSensor(String& json){

  windCounter = 0;
  timeL = millis();

  attachInterrupt(windPin,countWind,RISING);

  delay(1000 * measureTime);

  detachInterrupt(windPin);

  timeL = (millis() - timeL) / 1000;

  windSpeed = (float)windCounter / (float)measureTime * windFactor;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& fields = root.createNestedObject("values");
  fields["windCount"] = double_with_n_digits(windCounter, 2);
  fields["windSpeed"] = double_with_n_digits(windSpeed, 2);
  root["device"] = device;
  root["channel"] = channel;
  root["name"] = name;
  root.printTo(json);
}


void countWind() {
   windCounter ++;
}

void show_values(){
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Counts: "); display.print(windCounter); display.println("");
  display.print("Speed:  "); display.print(windSpeed); display.println(" km/h");
  display.display();
}