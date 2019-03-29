#include <Arduino.h>
/*
    This sketch establishes a TCP connection to a "quote of the day" service.
    It sends a "hello" message, and then prints received data.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <LiquidCrystal.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Wire.h>
#ifndef STASSID
  #define STASSID "YOUR WIFI NET"
  #define STAPSK  "YOUR PASSWORD"
#endif
#ifndef SERVICENAME
  #define SERVICENAME "RAS01"
  #define PASSPHRASE  "ras_01"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;
const String host = "http://192.168.1.67:8069";
const uint16_t port = 8069;
String service_name = SERVICENAME;
String passphrase = PASSPHRASE;
WiFiClient c;
#define SS_PIN 4  //D2
#define RST_PIN 5 //D1
#define LCD_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal lcd(LCD_PIN);
String printArray(byte *buffer, byte bufferSize) {
  String s = "";
  for (byte i = 0; i < bufferSize; i++) {
    s += String(buffer[i], HEX);
  }
  return s;
}

void setup() {
  Serial.begin(115200);
   while (! Serial){
     delay(1);
   }
  // We start by connecting to a WiFi network
  SPI.begin();      //Función que inicializa SPI

  lcd.begin(16, 2);
    // Print a message to the LCD.
    lcd.print("hello, world!");
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  mfrc522.PCD_Init();     //Función  que inicializa RFID
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  String card =  printArray(mfrc522.uid.uidByte, mfrc522.uid.size);

  mfrc522.PICC_HaltA();
  Serial.println(card);
    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, host + "/iot/"+service_name+"/action")) {  // HTTP
      Serial.print("connecting to ");
      Serial.print(host);
      Serial.print(':');
      Serial.println(port);
      String data = "passphrase=" + passphrase +"&value=" + card;
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST(data);
      if (httpCode <= 0) {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        DynamicJsonDocument doc(1000);
        Serial.println(payload);
        auto error = deserializeJson(doc, payload);
        if (error) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
        else {
          JsonObject object = doc.as<JsonObject>();
          String action = object["action"].as<String>();
          Serial.println(action);
        }
      }
      http.end();
      // Close the connection
      Serial.println();
      Serial.println("closing connection");
      client.stop();
    }

  delay(1000); // execute once every 5 minutes, don't flood remote service
}
