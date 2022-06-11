
//Library ardino json
#include <ArduinoJson.h>

//library lcd display
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//library software serial untuk komunikasi
#include <SoftwareSerial.h>
//pin software serial rx dan tx
SoftwareSerial linkSerial(5, 6);

//library eeprom untuk menyimpan nilai kalibrasi
#include <EEPROM.h>

int refresh = 0;

const int led_pin = 13;

const int btn_pin[3] = {8, 9, 7};

//variable untuk sensor ph
const int ph_pin = A0;
int ph_value = 0;
unsigned long avg_value;
int buffer_arr[10], temp;
float ph_volt, ph_act;
float kalibrasi_ph = 0;

//variable untuk sensor Turbidity
const int turb_pin = A1;
float turb_volt;
int turb_sample = 600;
float turb_ntu;
int turb_value;
float kalibrasi_turb;

int menu;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  lcd.init();

  pinMode(led_pin, OUTPUT);
  pinMode(ph_pin, INPUT);
  pinMode(turb_pin, INPUT);

  for (int i = 0; i < 5; i++) {
    pinMode(btn_pin[i], INPUT_PULLUP);
  }

  for (int i = 0; i < 8; i++) {
    digitalWrite(led_pin, !digitalRead(led_pin));
    delay(80);
  }

  kalibrasi_ph = EEPROM.read(0);    //membaca nilai kalibrasi ph dari eeprom
  kalibrasi_turb = EEPROM.read(1);  //membaca nilai kalibrasi turbidity dari eeprom

  linkSerial.begin(4800); //memulai komunikasi serial
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" IoT ThingSpeak");

  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (++refresh > 2) {
    lcd.clear();
    refresh = 0;
  }

  //short_long_btn();
  ph_read();
  turb_read();

  //buat format json untuk dikirim ke nodemcu
  StaticJsonDocument<500> docSending;
  docSending["ph_sensor"]   = ph_act;
  docSending["turb_sensor"] = turb_ntu;
  serializeJson(docSending, linkSerial);

  menu_display();

  lcd.setCursor(0, 0);
  lcd.print("PH :"); lcd.print(ph_act);
  lcd.setCursor(12, 0); lcd.print(ph_volt);
  lcd.setCursor(0, 1);
  lcd.print("NTU:"); lcd.print(turb_ntu - kalibrasi_turb);
  lcd.setCursor(12, 1); lcd.print(turb_volt);

  //  lcd.setCursor(0, 1); lcd.print(btn(0));
  //  lcd.print(" ");  lcd.print(btn(1));
  //  lcd.print(" ");  lcd.print(btn(2));
}

void menu_display() {
  if (btn(2) == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("MENU KALIBRASI"));
    delay(500);
    while (1) {
      if (++refresh > 5) {
        lcd.clear();
        refresh = 0;
      }
      if (btn(0) == 1) {
        menu++;
        delay(80);
      }
      if (btn(1) == 1) {
        menu--;
        delay(80);
      }
      if (menu > 2) menu = 0;
      if (menu < 0) menu = 2;
      if (menu == 0) {
        lcd.setCursor(0, 0); lcd.print("MENU KALIBRASI");
        lcd.setCursor(0, 1); lcd.print(menu); lcd.print(".ph kalibrasi");
        if (btn(2) == 1) {
          while (1) {
            if (++refresh > 5) {
              lcd.clear();
              refresh = 0;
            }
            lcd.setCursor(0, 0); lcd.print("KALIBRASI PH");
            lcd.setCursor(0, 1); lcd.print("nilai:"); lcd.print(kalibrasi_ph);
            if (btn(0) == 1) {
              kalibrasi_ph += 0.02;
              delay(80);
            }
            if (btn(1) == 1) {
              kalibrasi_ph -= 0.02;
              delay(80);
            }
            if (btn(2) == 1) {
              //delay(200);
              break;
            }
            led_indikator();
          }
        }
      }
      if (menu == 1) {
        lcd.setCursor(0, 0); lcd.print("MENU KALIBRASI");
        lcd.setCursor(0, 1); lcd.print(menu); lcd.print(".turb kalibrasi");
        if (btn(2) == 1) {
          delay(80);
          while (1) {
            if (++refresh > 5) {
              lcd.clear();
              refresh = 0;
            }
            lcd.setCursor(0, 0); lcd.print("KALIBRASI TURBID");
            lcd.setCursor(0, 1); lcd.print("nilai:"); lcd.print(kalibrasi_turb);
            if (btn(0) == 1) {
              kalibrasi_turb += 0.2;
              delay(80);
            }
            if (btn(1) == 1) {
              kalibrasi_turb -= 0.2;
              delay(80);
            }
            if (btn(2) == 1) {
              //delay(200);
              break;
            }
            led_indikator();
          }
        }
      }
      if (menu == 2) {
        lcd.setCursor(0, 0); lcd.print("MENU KALIBRASI");
        lcd.setCursor(0, 1); lcd.print(menu); lcd.print(".save & exit ?");
        if (btn(2) == 1) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(F("SAVE"));
          int save_eeprom;
          for (save_eeprom = 0; save_eeprom < 16; save_eeprom++) {
            lcd.print(".");
            delay(25);
          }
          if (save_eeprom > 5) {
            EEPROM.update(0, kalibrasi_ph);
            EEPROM.update(1, kalibrasi_turb);
          }
          break;
        }
      }
      led_indikator();
    }
  }
}

bool btn(int ch) {
  return digitalRead(btn_pin[ch]);
}

void led_indikator() {
  if (btn(0) == 1 || btn(1) == 1 || btn(2) == 1) {
    digitalWrite(led_pin, 1);
  } else {
    digitalWrite(led_pin, 0);
  }
}

void ph_read() {
  //Get 10 sample value from the sensor for smooth the value
  for (int i = 0; i < 10; i++)  {
    buffer_arr[i] = analogRead(ph_pin);
    delay(5);
  }
  //sort the analog from small to large
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  avg_value = 0;

  for (int i = 2; i < 8; i++) {
    avg_value += buffer_arr[i];
  }

  ph_volt = (float)avg_value * 5.18 / 1024 / 6; //convert the analog into millivolt
  ph_act = 2.5 * ph_volt - kalibrasi_ph; //convert the millivolt into pH value
}

void turb_read() {
  turb_value = analogRead(turb_pin);
  turb_volt = 0;

  for (int i = 0; i < turb_sample; i++) {
    turb_volt += ((float)analogRead(turb_pin) / 1023) * 5.1;
  }

  turb_volt = turb_volt / (turb_sample);
  turb_volt = round_to_dp(turb_volt, 2);

  if (turb_volt < 2.5) {
    turb_ntu = 3000;
  } else {
    turb_ntu = -1120.4 * square(turb_volt) + 5742.3 * turb_volt - 4352.8;
    if (turb_ntu < 0) turb_ntu = 0.5;
  }

  //turb_ntu = turb_ntu - kalibrasi_turb;

}

float round_to_dp( float in_value, int decimal_place ) {
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}

