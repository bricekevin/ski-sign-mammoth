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
#define LED_PIN 12
#define BUTTON_PIN 14
#define SWITCH_PIN 27
int buttonState = 0; // variable for reading the pushbutton status
int switchState = 0; // variable for reading the switch status (low winter left, high summer right)
#define COLOR_ORDER RGB
#define CHIPSET WS2811
#define NUM_LEDS 25
#define BRIGHTNESS 255
#define FRAMES_PER_SECOND 60
bool gReverseDirection = false;
bool firstLoop = true;
const int chairCount = 25;
int checkChair = 60000;
int chairMap = chairCount;
bool chairFound = false;
String path = "";

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

/*

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
*/

String liftNames[chairCount] = {
    "Broadway Express 1",
    "Canyon Express 16",
    "Chair 12",
    "Chair 13",
    "Chair 14",
    "Chair 20",
    "Chair 21",
    "Chair 22",
    "Chair 23",
    "Chair 25",
    "Chair 7",
    "Chair 8",
    "Cloud Nine Express 9",
    "Discovery Express 11",
    "Eagle Express 15",
    "Face Lift Express 3",
    "Gold Rush Express 10",
    "High 5 Express",
    "Panorama Lower",
    "Panorama Upper",
    "Roller Coaster Express 4",
    "Schoolyard Express 17",
    "Stump Alley Express 2",
    "Unbound Express 6",
    "Village Gondola"};

void setup(void)
{

  Serial.begin(115200);

  // ----- ----- Fast LED Setup ----- ----- //
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  // ----- ----- END Fast LED Setup ----- ----- //

  // HANDLE BUTTON & Switch
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);

  // Setup Wifi Manager
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  //wm.setClass("invert");   // set dark theme
  wm.setShowStaticFields(true); // force show static ip fields
  wm.setShowDnsFields(true);    // force show dns field always
  wm.setConfigPortalTimeout(180);
  buttonState = digitalRead(BUTTON_PIN);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW)
  {
    delay(50); //Poor Man Deboucne
    if (buttonState == LOW)
    {
      // button pressed
      Serial.println("--- Button Pressed in Setup");

      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::Black;
        FastLED.show();
      }
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::White;
        FastLED.show();
        delay(100);
        leds[chair] = CRGB::Black;
        FastLED.show();
      }
      wm.resetSettings();
      delay(1000);
      ESP.restart();
    }
  }
  /*
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
  */

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
    // drd->stop();
    setupOTA();
    CHK_FIRMWARE();
  }
}

void loop(void)
{

  // Check API Periodically and every frist itme
  if ((millis() - prevMillisHttp > checkChair) || (firstLoop == true))
  {
    firstLoop = false; // first time make call then wait

    switchState = digitalRead(SWITCH_PIN);

    if (switchState == LOW)
    {
      Serial.println("API: Winter Mode Enabled");
      path = "https://alterra.te2.biz/v1/conditions/ALT_MAM/lifts";
    }
    else if (switchState == HIGH)
    {
      Serial.println("API: Summer Mode Enabled");
      path = "https://alterra.te2.biz/v1/conditions/ALT_MAM_SUMMER/lifts";
    }
    String hostname = "alterra.te2.biz";
    client.setCACert(ca_cert);
    client.connect(hostname.c_str(), 443);

    https.setTimeout(20000);
    https.begin(client, path);
    https.addHeader("X-Customer-ID", "alterra");
    https.addHeader("Authorization", "Basic ZnVlbGVkX0FQSTp6YWg5b3RhbjVVZnVQb2hmZWVRdV5hZXF1b282YXQ2cXVpZV9qYWlobzRpZUc1am9vO0ZhZXhpZThqaWVzN1No");

    int httpCode = https.GET();
    Serial.print("API: Response Code: ");
    Serial.println(httpCode);

    // Emergency Restart
    if (httpCode != 200)
    {
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::White;
        FastLED.show();
      }
      delay(100);
      Serial.println("--- API: Emergency Restart");


    }

    // Reset Payload and String Count
    payload1 = "";
    payload1.reserve(15469);
    int count = 0;

    while (client.available())
    {

      payload1.concat((char)client.read());
      count++;
    }
    Serial.println("API: Payload Size: " + (String)count);
    // Serial.println(payload1);

    // Serial.println(payload1);

    DynamicJsonDocument doc(2000);

    // if (payload1.length() > 1000 && httpCode == 200)

    if (httpCode == 200)
    {
      DynamicJsonDocument filter(800);

      filter["lifts"][0]["name"] = true;
      // filter["lifts"][0]["status"] = true;
      filter["lifts"][0]["statusLabel"] = true;
      // filter["list"][0]["main"]["temp"] = true;

      // Deserialize the document
      deserializeJson(doc, payload1, DeserializationOption::Filter(filter));
      serializeJsonPretty(doc, Serial); // show the pretty JSON
      Serial.println("");

      /*
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::White;
        FastLED.show();
      }
*/
      // Check All JSON Returned Items (through cunt of lifts)
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        chairFound = false;

        // Check name in array for place
        for (int lift = 0; lift <= (chairCount - 1); lift++)
        {
          if (doc["lifts"][chair]["name"] == liftNames[lift])
          {
            chairMap = lift; // set to bigger than used LED Number
            chairFound = true;
          }
        }

        Serial.print("Chair Sequence: ");
        Serial.print(chair);
        Serial.print(" | Chair Map: ");
        Serial.print(chairMap);

        if (chairFound == true)
        {

          Serial.print(" | Chair Name: ");
          Serial.print(liftNames[chairMap]);

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
        else
        {
          Serial.println(" - NOT FOUND IN API");
        }
        chairMap = chairCount;
      }
    }

    Serial.println("API: END OF CHAIR LOOP");
    https.end();
    client.stop();
    client.flush();
    Serial.println("API: After CLIENT: END, STOP, FLUSH");
    prevMillisHttp = millis();
  }

  // Reconnect Wifi if Disconnected
  if (!WiFi.isConnected())
  {
    WiFi.reconnect();
    delay(5000);
  }

  // Periodically Check OTA
  if (millis() - prevMillsOTA > (checkChair * 5))
  {
    setupOTA();
    CHK_FIRMWARE();
    prevMillsOTA = millis();
  }
  // drd->loop();

  buttonState = digitalRead(BUTTON_PIN);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW)
  {
    delay(50); // Poor Man Debounce
    if (buttonState == LOW)
    {
      // button pressed
      Serial.println("--- Button Pressed in Setup");

      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::Black;
        FastLED.show();
      }
      for (int chair = 0; chair <= (chairCount - 1); chair++)
      {
        leds[chair] = CRGB::White;
        FastLED.show();
        delay(100);
        leds[chair] = CRGB::Black;
        FastLED.show();
      }
      WiFi.disconnect(false, true);
      delay(1000);
      ESP.restart();
    }
  }
} // End of Loop
