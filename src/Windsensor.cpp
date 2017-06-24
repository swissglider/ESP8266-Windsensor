// #################################################
// ## Framework include
// #################################################
#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

// needed for WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

// needed for OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// #################################################
// ## Application include
// #################################################
#include <ESP8266HTTPClient.h>
// includes OLED
#include <Adafruit_SSD1306.h>
#include <Wire.h>
// includes for JSON
#include <ArduinoJson.h>

// #################################################
// ## Framework parameter
// #################################################
const char* apName = "esp8266_Windsensor";
const char* hostName = "esp8266_Windsensor";
const char* passwort = "8266";

// #################################################
// ## Application parameter
// #################################################
// global parameter
#define SERIAL_BAUD 115200
String device = "ArduinoSensor";
String channel = "12";
String name = "Windsensor_Storen";

// wifi settings
String webSockerURI1 = "http://192.168.86.167:1880/ArduinoSensorWS";
HTTPClient http;

// parameter for OLED
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

// parameter for Windmesser
const int windPin = D4;          // anschluss des reedkontaktes
const float windFactor = 1;   // umrechnungsfaktor von umdrehungen in geschwindigkeit
const int measureTime = 5;      // messzeitraum in sekunden
volatile unsigned int windCounter = 0;  //interner zaehler f?r umdrehungen
float windSpeed = 0.0;
unsigned long timeL = 0;  // initialisieren der variablen f?r messwerte und die zeitmessung

// #################################################
// ## Application Functions
// #################################################
// **********************************************
// Write to WebSocket

void writeWebSocketMessage(String json, String sockerURI){
  http.begin(sockerURI);
  int httpCode = http.POST(json);
  http.end();
}

// **********************************************
// Write to WindSensor

//interrupt service routine f?r das zaehlen der umdrehungen
void countWind() {
   windCounter ++;
}

void readWindSensor(String& json){
  //zaehler auf 0 stellen
  windCounter = 0;
  timeL = millis();
  //zaehl-interrupt aktivieren
  attachInterrupt(windPin,countWind,RISING);
  //abwarten des messzeitraums
  delay(1000 * measureTime);
  //zaehl-interrupt deaktivieren
  detachInterrupt(windPin);
  //zeit bestimmen
  timeL = (millis() - timeL) / 1000;
  //berechnen der geschwindigkeit
  windSpeed = (float)windCounter / (float)measureTime * windFactor;

  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& fields = root.createNestedObject("values");
  fields["windCount"] = windCounter;
  fields["windSpeed"] = windSpeed;
  root["device"] = device;
  root["channel"] = channel;
  root["name"] = name;
  root.printTo(json);
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



void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

    // #################################################
    // ## Framework - WiFiManager
    // #################################################
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    wifiManager.autoConnect(apName, passwort);

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    // #################################################
    // ## Framework - OTA
    // #################################################
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(hostName);

    ArduinoOTA.setPassword(passwort);
      ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
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

  // #################################################
  // ## Start Application Setup here
  // #################################################
  //debug ausgaben auf die serielle schnittstelle
  Serial.begin(SERIAL_BAUD);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //startzeit bestimmen
  timeL = millis();

}

void loop() {
  // #################################################
  // ## Framework - OTA
  // #################################################
  ArduinoOTA.handle();

  // #################################################
  // ## Start Application loop here
  // #################################################
  // read Windsensor
  String wind_json = "";
  readWindSensor(wind_json);
  writeWebSocketMessage(wind_json, webSockerURI1);
  show_values();
}
