#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <MFRC522.h>
#include <ArduinoHttpClient.h>
#include <SD.h>
#include <Arduino_JSON.h>
char server[] = "192.168.1.68";
int port = 8069;
EthernetClient c;
HttpClient client = HttpClient(c, server, port);
LiquidCrystal lcd(9, 8, 19, 18, 17, 16);
#if defined(WIZ550io_WITH_MACADDRESS) // Use assigned MAC address of WIZ550io
;
#else
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
#endif
MFRC522 mfrc522(14, 15);

String printArray(byte *buffer, byte bufferSize) {
String s = "";
for (byte i = 0; i < bufferSize; i++) {
s += String(buffer[i], HEX);
}
return s;
}
void setup()
{
Serial.begin(9600);
while (!Serial) {
;
}
if (!SD.begin(4)) {
Serial.println("initialization failed!");
while (1);
}
lcd.begin(16, 2);
Serial.println(F("Starting....."));
SPI.begin();      //Función que inicializa SPI
lcd.print(F("Loading"));
mfrc522.PCD_Init();     //Función  que inicializa RFID
#if defined(WIZ550io_WITH_MACADDRESS)
Ethernet.begin();
#else
if (Ethernet.begin(mac) == 0) {
Serial.println("Failed to configure Ethernet using DHCP");
}
#endif
lcd.clear();
lcd.print("Loaded");
delay(1000);
}

void loop()
{
lcd.clear();
if (mfrc522.PICC_IsNewCardPresent())
{
if (mfrc522.PICC_ReadCardSerial())
{
String card =  printArray(mfrc522.uid.uidByte, mfrc522.uid.size);
mfrc522.PICC_HaltA();
lcd.print("Connecting");
String data = "passphrase=ras_01&value=" + card;
client.post("/iot/RAS01/action", "application/x-www-form-urlencoded", data);
lcd.clear();
lcd.print(client.responseBody());
delay(1000);
lcd.clear();
}
}
}
