//library arduino json
#include <ArduinoJson.h>

//library software serial
#include <SoftwareSerial.h>
SoftwareSerial linkSerial(12, 13); // RX, TX

//library wifi
#include <ESP8266WiFi.h>

//masukin key buat think speak
#include "secrets.h"
//library thingspeak
#include "ThingSpeak.h"

char ssid[] = SECRET_SSID; //nama wifi
char pass[] = SECRET_PASS; //password wifi

int keyIndex = 0;

WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

String myStatus = "";

const int led_pin = 16;

int refresh = 0;

float ph_kiriman;
float turb_kiriman;

//interval untuk mengirim ke thingspeak
unsigned long last_time = 0;
unsigned long timer_delay = 30000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);

  for (int i = 0; i < 8; i++) {
    digitalWrite(led_pin, digitalRead(led_pin));
    delay(80);
    Serial.println(".");
  }

  linkSerial.begin(4800);

  while (!Serial) {
    ;
  }

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  WiFi.begin(ssid, pass);

  //cek koneksi wifi
  konek_wifi();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (linkSerial.available()) {

    StaticJsonDocument<500> docReceive;
    //deserializeJson(doc, json);

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(docReceive, linkSerial);

    //mengambil kiriman nilai dari arduino
    if (err == DeserializationError::Ok) {
      ph_kiriman    = docReceive["ph_sensor"].as<float>();
      turb_kiriman  = docReceive["turb_sensor"].as<float>();
    }

    else {
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());
      // Flush all bytes in the "link" serial port buffer
      while (linkSerial.available() > 0)
        linkSerial.read();
    }

    Serial.print("ph kiriman: "); Serial.print(ph_kiriman);
    Serial.print("\tturb kiriman: "); Serial.println(turb_kiriman);
    
    ThingSpeak.setField(1, ph_kiriman);
    ThingSpeak.setField(2, turb_kiriman);
    ThingSpeak.setField(3, WiFi.RSSI());

    //mengirim variable sensor ke thingsepak setiap 30 detik
    if ((millis() - last_time) > timer_delay) {
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (x == 200) {
        Serial.println("Channel update successful.");
      }
      else {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
      last_time = millis();
    }
  } else {
    //Serial.println("Serial Error");
    //delay(100);
  }
}

void konek_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      //WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      //lcd.setCursor(0, 0);
      //lcd.print("con to network");
      //membuat .... pada lcd
      if (++refresh >= 0) {
        //lcd.setCursor(refresh, 1);
        //lcd.print(".");
        delay(100);
        if (refresh == 16) {
          refresh = -1;
          //lcd.clear();
        }
      }
      digitalWrite(led_pin, digitalRead(led_pin) ^ 1); //blink led
    }
    Serial.println("\nConnected.");
  }
}
