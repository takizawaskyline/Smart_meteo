#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <GyverBME280.h>
#include <ArduinoJson.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
GyverBME280 bme;

const char* ssid = "Keenetic-3618";
const char* password = "bzwCZBUk";

const char* mqtt_server = "srv2.clusterfly.ru";
const int mqtt_port = 9991;
const char* mqtt_user = "user_159400dd";
const char* mqtt_pass = "IsAXcJDu4RQA4";
const char* mqtt_topic_sens_pub = "user_159400dd/sens_val";

WiFiClient espClient;
PubSubClient client(espClient);
bool mqttConnected = false;  // Флаг подключения

void setup() {
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

  client.loop();
  //Публикация сообещния
  char msg[10];
  int a = 22;
  String data_json = sens_val();
  client.publish(mqtt_topic_sens_pub, data_json.c_str());
  Serial.println(data_json);
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
  String json = json_file(&val_h, &val_temp, &val_pressure);

  return json;
}


String json_file(int* val_h, int* val_temp, int*val_pressure) {
  String jsonchik = "{";
  jsonchik += "\"sens_val\": {";
  jsonchik += "\"hum\":" + String(*val_h) + ",";
  jsonchik += "\"temp\":" + String(*val_temp) + ",";
  jsonchik += "\"pressure\":" + String(*val_pressure);
  jsonchik += "}}";

  return jsonchik;
}
