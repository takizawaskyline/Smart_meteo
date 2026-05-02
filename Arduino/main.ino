#include <Wire.h> 
#include <TroykaMQ.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <GyverBME280.h>
#include <ArduinoJson.h>
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
GyverBME280 bme;


const char* ssid = "Keenetic-3618";
const char* password = "bzwCZBUk";

const char* mqtt_server = "m4.wqtt.ru";
const int mqtt_port = 15934;
const char* mqtt_user = "u_1R9J5Q";
const char* mqtt_pass = "aC0jfFJi";
const char* mqtt_topic_sens_pub = "user_159400dd/sens_val";

int hydrogen = 0;
int smoke = 0;
int methane = 0;
int LPG = 0;
int ratio = 0;


WiFiClient espClient;

PubSubClient client(espClient);
bool mqttConnected = false;  // Флаг подключения

#define PIN_MQ2 34
#define PIN_MQ2_HEATER  26
MQ2 mq2(PIN_MQ2, PIN_MQ2_HEATER);

void setup() {
  mq2.heaterPwrHigh();
  
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

void loop() {
     if (!mq2.isCalibrated() && mq2.heatingCompleted()) {
    // выполняем калибровку датчика на чистом воздухе
    mq2.calibrate();
    // выводим сопротивление датчика в чистом воздухе (Ro) в serial-порт
    Serial.print("Ro = ");
    Serial.println(mq2.getRo());
  }
     if (mq2.isCalibrated() && mq2.heatingCompleted()) {
    hydrogen = mq2.readHydrogen();
    smoke = mq2.readSmoke();
    methane = mq2.readMethane();
    LPG = mq2.readLPG();
    ratio = mq2.readRatio();
  }
  mqtt_connect(); 
  client.loop();
  //Публикация сообещния
  char msg[10];
  int a = 22;
  String data_json = sens_val();
  client.publish(mqtt_topic_sens_pub, data_json.c_str());
  Serial.println(data_json);
  otladka();
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
  Serial.println(bme.readPressure() / 133,32);

  Serial.print("Ro = ");
  Serial.println(mq2.getRo());

  if (mq2.isCalibrated() && mq2.heatingCompleted()) {
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
  }
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
  
  // Пытаемся переподключаться не чаще 1 раза в 5 секунд
  if (millis() - lastAttempt < 5000) {
    return;
  }
  lastAttempt = millis();
  
  Serial.print("Попытка подключения к MQTT...");
  
  // Случайный ID клиента для избежания конфликтов
  String clientId = "ESP32-" + String(random(0xFFFF), HEX);
  
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println("Успешно!");
    //client.subscribe(mqtt_topic_sub);
  } else {
    Serial.print("Ошибка, rc=");
    Serial.println(client.state());
  }
}


String sens_val() {
  int val_temp = bme.readTemperature();
  int val_h = bme.readHumidity();
  int val_pressure = bme.readPressure() / 133.3;


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
