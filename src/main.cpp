#include <Arduino.h>
#include <FS.h> // Manage the nodeMCU filesystem
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <WiFiManager.h>
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
SSD1306  display(0x3c, 0, 2);
WiFiManager wifiManager;
ESP8266WebServer *server;

String ssid     = "TEST_RAS";
String password = "test_ras";
String host;
String service_name;
String passphrase;
WiFiClient c;
bool configured = false;
#define SS_PIN 4  //D2
#define RST_PIN 5 //D1
MFRC522 mfrc522(SS_PIN, RST_PIN);
String printArray(byte *buffer, byte bufferSize) {
  String s = "";
  for (byte i = 0; i < bufferSize; i++) {
    s += String(buffer[i], HEX);
  }
  return s;
}
void sleep(){
  ESP.deepSleep(1e6);
}
void drawInitializing()
{
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(10, 0, "Initializing...");
}
void drawSoftAP(){
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 10, "Connect to " + ssid);
    String  MyIp;
    MyIp =  String(WiFi.softAPIP()[0]) + "." + String(WiFi.softAPIP()[1]) + "." + String(WiFi.softAPIP()[2]) + "." + String(WiFi.softAPIP()[3]);
    display.drawString(0, 30, "Access " + MyIp);
}
void submit() {
  ssid = server->arg("ssid");
  password = server->arg("password");
  String url = server->arg("odoo_link");
  server->send(200, "text/html", "<html><body><<h1>Configuring</h1></body></html>");
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int i = 1;
  Serial.println(ssid);
  Serial.println(password);
  while (i<20 && WiFi.status() != WL_CONNECTED) {
    delay(500);
    i += 1;
    Serial.print(".");
  }
  if (i>= 20){
    Serial.println("Configuration failed");
    sleep();
  }
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, url)) {  // HTTP
    Serial.print("connecting to ");
    Serial.println(url);
    String data = "template=";
    data += "eficent.ras";
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
  sleep();
}
void handleRoot() {
  Serial.println("Handle Root");
  String temp = "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>RAS Attendance System configuration Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello to RAS Attendance System</h1>\
    <p>Fille the following form</p>\
    <form action='/submit' method='post'>\
      Wifi SSID:<br/>\
      <input type='text' name='ssid'/><br/>\
      Password:<br/>\
      <input type='text' name='password'/><br/>\
      Odoo link<br/>\
      <input type='text' name='odoo_link'/><br/>\
      <input type='submit' value='Submit'>\
    </form>\
  </body>\
</html>";
  server->send(200, "text/html", temp);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";

  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
}

void setup() {
  Serial.begin(9600);
  // We start by connecting to a WiFi network
  SPI.begin();      //Función que inicializa SPI
  Serial.println("mounting FS...");
  display.init(); // Initialising the UI will init the display too.
  display.flipScreenVertically();

  display.clear();
  drawInitializing();
  display.display();
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
          //file exists, reading and loading
          Serial.println("reading config file");
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile) {
            Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonDocument doc(1000);
            auto error = deserializeJson(doc, buf.get());
            if (error) {
                Serial.print(F("deserializeJson() failed with code "));
                Serial.println(error.c_str());
            }
            else {
              JsonObject json = doc.as<JsonObject>();
              Serial.println("\nparsed json");
              host = json["host"].as<String>();
              ssid = json["ssid"].as<String>();
              password = json["password"].as<String>();
              service_name = json["service_name"].as<String>();
              passphrase = json["passphrase"].as<String>();
              configured = true;
            }
          }
        }
      } else {
        Serial.println("failed to mount FS");
    }
  }
}
if (configured){
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
else {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  server = new ESP8266WebServer(80);
  server->on("/", handleRoot);
  server->on("/submit", submit);
  server->on("/inline", []() {
    server->send(200, "text/plain", "this works as well");
  });
   server->begin();
   display.clear();
   drawSoftAP();
   display.display();
}
}

void loop() {
  if (! configured){
    server->handleClient();
    return;
  }
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
  String url = "";
  url = url + host;
  url = url + "/iot/"+service_name+"/action";
  if (http.begin(client, url)) {  // HTTP
    Serial.print("connecting to ");
    Serial.print(host);
    String data = "passphrase=";
    data = data + passphrase;
    data = data + "&value=";
    data = data + card;
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
