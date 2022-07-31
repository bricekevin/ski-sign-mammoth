//Required esp32c fixes: 
// (A) asyncy fix from https://github.com/me-no-dev/ESPAsyncWebServer/issues/1101
// (B) return on little spiffs 

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "OTA.h"
unsigned long prevMillisHttp = 0;
// #include <ESPAsyncWebServer.h>
// #include <AsyncTCP.h>
// #include "SPIFFS.h"
unsigned long prevMillsOTA = 0;
String response = "";
WiFiClientSecure client;
HTTPClient https;
String payload1;
#define LED_PIN 10
#define COLOR_ORDER RGB
#define CHIPSET WS2811
#define NUM_LEDS 25
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 60
bool gReverseDirection = false;
bool firstLoop = true;
bool summerMode = true;
int chairCount = 25;
int chairMap = chairCount;

CRGB leds[NUM_LEDS];

const char *ca_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// For ESP8266, You must select one to be true (RTC, EEPROM or SPIFFS)
// Otherwise, library will use default EEPROM storage
#define ESP_DRD_USE_EEPROM true
#define ESP_DRD_USE_SPIFFS false // false

#ifdef ESP8266
#define ESP8266_DRD_USE_RTC false // true
#endif

#define DOUBLERESETDETECTOR_DEBUG false // false

#include <ESP_DoubleResetDetector.h> //https://github.com/khoih-prog/ESP_DoubleResetDetector

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0x54

DoubleResetDetector *drd;

void setup(void)
{

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  if (drd->detectDoubleReset())
  {
    Serial.println("--- Double Reset Detected");
    wm.resetSettings();
    delay(1000);
    ESP.restart();
  }
  else
  {
    Serial.println("--- No Double Reset Detected");
  }

  bool res;
  res = wm.autoConnect("SETUP SKI SIGN", "123456789"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("CONNECTED.... :)");
    drd->stop();
  //  setupOTA(); //OTA not yet supported
  //  CHK_FIRMWARE(); //OTA not yet supported
  }

  // ----- ----- Fast LED Setup ----- ----- //
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  // ----- ----- END Fast LED Setup ----- ----- //
}

void loop(void)
{
  if ((millis() - prevMillisHttp > 60000) || (firstLoop == true))
  {
    firstLoop = false; // first time make call then wait
    String path = "https://alterra.te2.biz/v1/conditions/ALT_MAM_SUMMER/lifts";
    String hostname = "alterra.te2.biz";
    client.setCACert(ca_cert);
    client.connect(hostname.c_str(), 443);

    https.setTimeout(20000);
    https.begin(client, path);
    https.addHeader("X-Customer-ID", "alterra");
    https.addHeader("Authorization", "Basic ZnVlbGVkX0FQSTp6YWg5b3RhbjVVZnVQb2hmZWVRdV5hZXF1b282YXQ2cXVpZV9qYWlobzRpZUc1am9vO0ZhZXhpZThqaWVzN1No");

    int httpCode = https.GET();
    Serial.print("Response Code: ");
    Serial.println(httpCode);

    if (httpCode != 200)
    {
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::White;
        FastLED.show();
      }
      delay(100);
      Serial.print("--- Emergency Restart");

      ESP.restart();
    }

    payload1.reserve(15469);
    int count = 0;

    while (client.available())
    {

      payload1.concat((char)client.read());
      count++;
    }
    Serial.println("count: " + (String)count);
    // Serial.println(payload1);

    // Serial.println(payload1);

    DynamicJsonDocument doc(2000);
    if (payload1.length() > 1000 && httpCode == 200)
    {
      DynamicJsonDocument filter(800);

      filter["lifts"][0]["name"] = true;
      // filter["lifts"][0]["status"] = true;
      filter["lifts"][0]["statusLabel"] = true;
      // filter["list"][0]["main"]["temp"] = true;

      // Deserialize the document
      deserializeJson(doc, payload1, DeserializationOption::Filter(filter));
      serializeJsonPretty(doc, Serial); // show the pretty JSON

      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::Red;
        FastLED.show();
      }

      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        if (summerMode == true)
        {

          switch (chair)
          {
          case 0:
            chairMap = 13;
            break;

          case 1:
            chairMap = 18;
            break;

          case 2:
            chairMap = 19;
            break;

          case 3:
            chairMap = 22;
            break;

          case 4:
            chairMap = 1;
            break;
          default:
            chairMap = 25;
            break;
          }
        }
        else if (summerMode == false)
        {

          switch (chair)
          {
          case 0:
            chairMap = 0;
            break;

          case 1:
            chairMap = 1;
            break;

          case 2:
            chairMap = 2;
            break;

          case 3:
            chairMap = 3;
            break;

          case 4:
            chairMap = 4;
            break;

          case 5:
            chairMap = 5;
            break;

          case 6:
            chairMap = 6;
            break;

          case 7:
            chairMap = 7;
            break;

          case 8:
            chairMap = 8;
            break;

          case 9:
            chairMap = 9;
            break;

          case 10:
            chairMap = 10;
            break;

          case 11:
            chairMap = 11;
            break;

          case 12:
            chairMap = 12;
            break;

          case 13:
            chairMap = 13;
            break;

          case 14:
            chairMap = 14;
            break;

          case 15:
            chairMap = 15;
            break;

          case 16:
            chairMap = 16;
            break;

          case 17:
            chairMap = 17;
            break;

          case 18:
            chairMap = 18;
            break;

          case 19:
            chairMap = 19;
            break;

          case 20:
            chairMap = 20;
            break;

          case 21:
            chairMap = 21;
            break;

          case 22:
            chairMap = 22;
            break;

          case 23:
            chairMap = 23;
            break;

          case 24:
            chairMap = 24;
            break;
          default:
            chairMap = chair;
            break;
          }
        }

        Serial.print("Chair Sequence: ");
        Serial.print(chair);
        Serial.print(" | Chair Map: ");
        Serial.print(chairMap);

        if (doc["lifts"][chair]["statusLabel"] == "Open")
        {
          Serial.println(" - Open");
          leds[chairMap] = CRGB::Green;
          FastLED.show();
        }

        else if ((doc["lifts"][chair]["statusLabel"] == "Expected") || (doc["lifts"][chair]["statusLabel"] == "Hold") || (doc["lifts"][chair]["statusLabel"] == "Weather Hold") || (doc["lifts"][chair]["statusLabel"] == "Anticipated Weather Impact"))
        {
          Serial.println(" - Hold");
          leds[chairMap] = CRGB::DarkBlue;
          FastLED.show();
        }

        else if (doc["lifts"][chair]["statusLabel"] == "Closed")
        {
          Serial.println(" - Closed");
          leds[chairMap] = CRGB::Red;
          FastLED.show();
        }

        else
        {
          Serial.println(" - Closed");
          leds[chairMap] = CRGB::Red;
          FastLED.show();
        }
      }
    }

    Serial.println("--- END OF CHAIR LOOP");
    https.end();
    client.stop();
    client.flush();
    Serial.println("--- After CLIENT: END, STOP, FLUSH");
    prevMillisHttp = millis();
  }
  if (!WiFi.isConnected())
  {
    WiFi.reconnect();
    delay(5000);
  }
  if (millis() - prevMillsOTA > 120000)
  {
    //setupOTA(); //OTA not yet supported
    //CHK_FIRMWARE(); //OTA not yet supported
    prevMillsOTA = millis();
  }
  drd->loop();
}
/*
#include "WiFiManager.h"
//WiFiManager wifiManager;
//WiFiManager wm;

#define WEBSERVER_H
#include "ESPAsyncWebServer.h"




#include <WiFi.h>
#include <WiFiClientSecure.h>
//#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "OTA.h"
unsigned long prevMillisHttp=0;
// #include <ESPAsyncWebServer.h>
// #include <AsyncTCP.h>
// #include "SPIFFS.h"
unsigned long prevMillsOTA=0;
String response = "";
WiFiClientSecure client;
HTTPClient https;
String payload1;
#define LED_PIN 10
#define COLOR_ORDER RGB
#define CHIPSET WS2811
#define NUM_LEDS 25
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 60
bool gReverseDirection = false;

CRGB leds[NUM_LEDS];

const char *ca_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

void setup(void)
{

  Serial.begin(115200);
  Serial.println("---Setup Starting Delay 1000 ---");



  delay(1000);

// WiFi.disconnect(); //no change 

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
    Serial.println("--- Post WM Initialization Delay 1000 ---");

    delay(1000);

  bool res;
  res = wm.autoConnect("SETUP BRICE SKI", "12345678"); // password protected ap

  if (!res)
  {
    Serial.println("--- Failed to connect--- ");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("--- CONNECTED.... :)");
 //   setupOTA();
 //  CHK_FIRMWARE();
       Serial.println("--- DONE OTA--- ");

  }

  // ----- ----- Fast LED Setup ----- ----- //
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
         Serial.println("--- DONE FAST LED--- ");

  // ----- ----- END Fast LED Setup ----- ----- //
}

void loop(void)
{
 //          Serial.println("--- START OF LOOP ---");

if(millis()-prevMillisHttp>20000 && WiFi.isConnected()==true)
{
             Serial.println("MILLS > 20000");

  String path = "https://alterra.te2.biz/v1/conditions/ALT_MAM/lifts";
  String hostname = "alterra.te2.biz";
  client.setCACert(ca_cert);
  client.connect(hostname.c_str(), 443);

  https.setTimeout(20000);
  https.begin(client, path);
  https.addHeader("X-Customer-ID", "alterra");
  https.addHeader("Authorization", "Basic ZnVlbGVkX0FQSTp6YWg5b3RhbjVVZnVQb2hmZWVRdV5hZXF1b282YXQ2cXVpZV9qYWlobzRpZUc1am9vO0ZhZXhpZThqaWVzN1No");

  int httpCode = https.GET();
  Serial.print("Response Code: ");
  Serial.println(httpCode);
  Serial.println("Break 2");

  payload1.reserve(15469);
  int count = 0;

  while (client.available())
  {

    payload1.concat((char)client.read());
    count++;
  }
  Serial.println("count: " + (String)count);

//   Serial.println(payload1);

  // Serial.println(payload1);

  DynamicJsonDocument doc(2000);
  if (payload1.length() > 1000 && httpCode==200)
  {
    DynamicJsonDocument filter(800);

    filter["lifts"][0]["name"] = true;
    // filter["lifts"][0]["status"] = true;
    filter["lifts"][0]["statusLabel"] = true;
    // filter["list"][0]["main"]["temp"] = true;

    // Deserialize the document

    deserializeJson(doc, payload1, DeserializationOption::Filter(filter));
    serializeJsonPretty(doc, Serial);
  
  
  for (int chair = 0; chair <= 24; chair++)
  {

    Serial.print("Chair Sequence: ");
    Serial.print(chair);

    if (doc["lifts"][chair]["statusLabel"] == "Open")
    {
      Serial.println(" Open");
      leds[chair] = CRGB::Green;
      FastLED.show();
    }

    else if ((doc["lifts"][chair]["statusLabel"] == "Expected") || (doc["lifts"][chair]["statusLabel"] == "Hold") || (doc["lifts"][chair]["statusLabel"] == "Weather Hold") || (doc["lifts"][chair]["statusLabel"] == "Anticipated Weather Impact"))
    {
      Serial.println(" Hold");
      leds[chair] = CRGB::DarkBlue;
      FastLED.show();
    }

    else if (doc["lifts"][chair]["statusLabel"] == "Closed")
    {
      Serial.println(" Closed");
      leds[chair] = CRGB::Red;
      FastLED.show();
    }
    Serial.println("After Last Color Else");
  }
  }
  
  Serial.println("After Close 24 For");
  https.end();
  client.stop();
  client.flush();
  Serial.println("After HTTP END");
  Serial.println("END LOOP");
 prevMillisHttp=millis();
}
if(!WiFi.isConnected())
{
  WiFi.reconnect();
  delay(5000);
}
if(millis()-prevMillsOTA>120000)
{
    setupOTA();
   CHK_FIRMWARE();
   prevMillsOTA=millis();
}

}
*/