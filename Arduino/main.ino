#include <Wire.h> 
#include <TroykaMQ.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <GyverBME280.h>
#include <ArduinoJson.h>
#include "time.h"
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
GyverBME280 bme;


const char* ssid = "Keenetic-3618";
const char* password = "bzwCZBUk";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 10800; // Смещение для Липецка (UTC+3): 3 * 3600 = 10800
const int   daylightOffset_sec = 0; // Летнее время (в РФ не используется)
const char* mqtt_server = "m4.wqtt.ru";
const int mqtt_port = 15934;
const char* mqtt_user = "u_1R9J5Q";
const char* mqtt_pass = "aC0jfFJi";
const char* mqtt_topic_sens_pub = "user_159400dd/sens_val";
unsigned long lastDisplayUpdate = 0;
bool displayPage = 0;

int hydrogen = 0;
int smoke = 0;
int methane = 0;
int LPG = 0;
int ratio = 0;
int val_temp = 0;
int val_h = 0;
int val_pressure = 0;

WiFiClient espClient;

PubSubClient client(espClient);
bool mqttConnected = false;  // Флаг подключения

#define PIN_MQ2 34

MQ2 mq2(PIN_MQ2);

void setup() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  mq2.calibrate();
  bme.begin(0x77);
  if (!bme.begin(0x77)) Serial.println("Error!");
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(256);
  Serial.begin(9600);

  lcd.begin();    
  lcd.backlight();
  lcd.setCursor(0,0);

  wifi();
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time Error";
  }

  char timeString[20];
  // %H:%M — ЧАС:МИН
  strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
  
  return String(timeString);
}

String getFormattedDay() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time Error";
  }

  char timeString[20];
  // %H:%M — ЧАС:МИН
  strftime(timeString, sizeof(timeString), "%d.%m", &timeinfo);
  
  return String(timeString);
}


void loop() {
  mqtt_connect(); 
  client.loop();
  String data_json = sens_val();
  client.publish(mqtt_topic_sens_pub, data_json.c_str());
  Serial.println(data_json);
  otladka();
  lcd_main();
  delay(1000);

}


void mqtt_connect() {
    Serial.println();
      if (!client.connected()) {
    mqttConnected = false;
    reconnectMQTT();
  } else {
    if (!mqttConnected) {
      mqttConnected = true;
      Serial.println("MQTT подключён стабильно!");
      lcd.clear();
      lcd.print("MQTT connected!");
      delay(2000);
      lcd.clear();
    }
  }
}

void otladka() {
  // температура
  Serial.print("Temperature: ");
  Serial.println(bme.readTemperature());

  // влажность
  Serial.print("Humidity: ");
  Serial.println(bme.readHumidity());

  // давление
  Serial.print("Pressure: ");
  Serial.println(bme.readPressure() / 133.32);

  Serial.print("Ro = ");
  Serial.println(mq2.getRo());

  
    // выводим отношения текущего сопротивление датчика
    // к сопротивлению датчика в чистом воздухе (Rs/Ro)
    Serial.print("Ratio: ");
    Serial.print(mq2.readRatio());
    // выводим значения газов в ppm
    Serial.print("LPG: ");
    Serial.print(mq2.readLPG());
    Serial.print(" ppm ");
    Serial.print(" Methane: ");
    Serial.print(mq2.readMethane());
    Serial.print(" ppm ");
    Serial.print(" Smoke: ");
    Serial.print(mq2.readSmoke());
    Serial.print(" ppm ");
    Serial.print(" Hydrogen: ");
    Serial.print(mq2.readHydrogen());
    Serial.println(" ppm ");
    //delay(100);
    Serial.print("Calibrated: "); Serial.println(mq2.isCalibrated());
  
}

void wifi() {
  Serial.println();
  Serial.print("Подключение к ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi подключён");
  Serial.print("WiFi connected IP: ");
  Serial.println(WiFi.localIP());
  lcd.print("IP: ");
  lcd.print(WiFi.localIP().toString());  // toString() преобразует IP в строку
  delay(2000);
  lcd.clear();
}

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) {
    return;
  }
  lastAttempt = millis();
  
  Serial.print("Попытка подключения к MQTT...");
  String clientId = "ESP32-" + String(random(0xFFFF), HEX);
  
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println("Успешно!");
  } else {
    Serial.print("Ошибка, rc=");
    Serial.println(client.state());

    lcd.print("MQTT Err:");
    lcd.print(client.state());
    delay(2000);
    lcd.clear();
  }
}


String sens_val() {
  hydrogen = mq2.readHydrogen();
  smoke = mq2.readSmoke();
  methane = mq2.readMethane();
  LPG = mq2.readLPG();
  ratio = mq2.readRatio();
  val_temp = bme.readTemperature();
  val_h = bme.readHumidity();
  val_pressure = bme.readPressure() / 133.3;


  String json = json_file(&val_h, &val_temp, &val_pressure, &hydrogen, &smoke, &methane, &LPG, &ratio);

  return json;
}


String json_file(int* val_h, int* val_temp, int* val_pressure, int* hydrogen, int* smoke, int* methane, int* LPG, int* ratio) {
  String jsonchik = "{";
  jsonchik += "\"sens_val\": {";
  jsonchik += "\"hum\":" + String(*val_h) + ",";
  jsonchik += "\"temp\":" + String(*val_temp) + ",";
  jsonchik += "\"pressure\":" + String(*val_pressure) + ",";  // ← Добавлена запятая
  jsonchik += "\"hydrogen\":" + String(*hydrogen) + ",";
  jsonchik += "\"smoke\":" + String(*smoke) + ",";
  jsonchik += "\"methane\":" + String(*methane) + ",";
  jsonchik += "\"LPG\":" + String(*LPG) + ",";
  jsonchik += "\"ratio\":" + String(*ratio);
  jsonchik += "}}";

  return jsonchik;
}

void lcd_first_page() {
    lcd.clear();
    lcd.setCursor(11,0);

    lcd.print("T:");
    lcd.print(String(val_temp) + "C");

    lcd.setCursor(6,0);
    lcd.print("H:");
    lcd.print(String(val_h) + "%");

    lcd.setCursor(0,0);
    lcd.print(getFormattedTime());

    lcd.setCursor(0,1);
    lcd.print(getFormattedDay());

    lcd.setCursor(6,1);
    lcd.print("P:");
    lcd.print(String(val_pressure) + "mmGh");
}

void lcd_second_page(){
    lcd.clear();

    lcd.setCursor(6,0);
    lcd.print("S:");
    lcd.print(String(smoke) + "ppm");

    lcd.setCursor(0,0);
    lcd.print(getFormattedTime());

    lcd.setCursor(0,1);
    lcd.print(String(methane) + "ppm");

    lcd.setCursor(8,1);
    lcd.print("H:");
    lcd.print(String(hydrogen) + "ppm");
  }

  void lcd_main() {
    if (millis() - lastDisplayUpdate > 5000) {
    displayPage = !displayPage;
    lastDisplayUpdate = millis();
    lcd.clear();

    if (displayPage == 0) {
      lcd_first_page();
    }else {
      lcd_second_page();
    }


  }
}

