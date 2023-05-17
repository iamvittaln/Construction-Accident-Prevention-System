#include <SPI.h>
#include <MFRC522.h>

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
 
#define SS_PIN 21
#define RST_PIN 22
#define LED_G 2 //define green LED pin
#define LED_R 12 //define red LED
#define BUZZER 13 //buzzer pin

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

#define WIFI_SSID "Ideapad3 - Windows"
#define WIFI_PASSWORD "12345678"

#define API_KEY "AIzaSyA328XAjBnlh7zLhlL4KDg0cU1KZWxlYyM"
#define DATABASE_URL "rfid-attendance-system-fbc24-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK=false;
FirebaseJson json;

void connectToWifi()
{
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}


void setup()
{
  Serial.begin(115200);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER,LOW);

  //NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  connectToWifi();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Connected to Firebase!");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Put your card to the RFID reader...");
  Serial.println();
}
void loop()
{
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();

  digitalWrite(LED_R,HIGH);
  digitalWrite(BUZZER,HIGH);

   //NTP Time
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  
  //Time
  char timeHour[3],timeMinute[3],timeSeconds[3];
  strftime(timeHour,3, "%H", &timeinfo);
  strftime(timeMinute,3, "%M", &timeinfo);
  strftime(timeSeconds,3, "%S", &timeinfo);

  String hour=String(timeHour);
  String min=String(timeMinute);
  String sec=String(timeSeconds);

  String time=hour+":"+min+":"+sec;

  //Date
  char timeDay[3],timeMonth[20],timeWeekDay[10],timeYear[5];
  strftime(timeDay,3, "%d", &timeinfo);
  strftime(timeMonth,20, "%B", &timeinfo);
  strftime(timeYear,5, "%Y", &timeinfo);
  strftime(timeWeekDay,10, "%A", &timeinfo);

  String day=String(timeDay);
  String month=String(timeMonth);
  String year=String(timeYear);  
  String weekday=String(timeWeekDay);

  String date=day+"-"+month+"-"+year+", "+weekday; 
   
  String pth="Unauthorized/"+content;
  json.set("UID",content);
  json.set("TimeStamp",time);
  json.set("Date",date);
  json.set("Zone","Moving Vehicle");

  if (Firebase.RTDB.setJSON(&fbdo, pth, &json ))
  {
      Serial.println("Data sent to Firebase!");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
  }  
  else
  {
    Serial.println("Data not sent to Firebase!");
    Serial.println("REASON: " + fbdo.errorReason());
  }
  delay(3000);
  digitalWrite(LED_R,LOW);
  digitalWrite(BUZZER,LOW);
} 
