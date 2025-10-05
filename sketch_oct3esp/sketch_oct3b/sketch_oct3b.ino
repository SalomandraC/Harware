#include "DHT.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include <RH_ASK.h>
#include <SPI.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#define DHTPIN 33
#define DHTTYPE DHT11

RH_ASK rfdriver(2000, 5, 0, 0);

Preferences preferences_fire, preferences_temperature, preferences_had, preferences_servo;

enum class SensorType {
    TEMPERATURE = 0,
    HUMIDITY = 1,
    ALERT = 2,
    FIRE = 3
};


enum class AlertType {
    STUDY,
    ERROR,
    TEMPERATURE,
    HUMIDITY,
    MOTION,
    BATTERY
};

enum class Alert {
    SOUND,
    LIGHT,
    SLIGHT,
    NONE_ALERT
};


void sendSensorData(SensorType type, float value, const char* unit);
void sendAlertData(AlertType alert_type, const char* message, const char* severity);

String getSensorTypeName(SensorType sensor) {
    switch (sensor) {
        case SensorType::TEMPERATURE:
            return "temperature";
        case SensorType::HUMIDITY:
            return "humidity";
        case SensorType::ALERT:
            return "alert";
        case SensorType::FIRE:
            return "fire";
        default:
            return "UNKNOWN";
    }
}


String getAlertMessage(AlertType alertA) {
    switch (alertA) {
        case AlertType::STUDY:
            return "study";
        case AlertType::ERROR:
            return "error";
        case AlertType::TEMPERATURE:
            return "temperature";
        case AlertType::HUMIDITY:
            return "humidity level critical";
        case AlertType::MOTION:
            return "motion";
        case AlertType::BATTERY:
            return "battery";
        default:
            return "Unknown alert";
    }
}

void saveMaxFire(float value) {
  preferences_fire.begin("fire-settings", false); 
  preferences_fire.putInt("maxFire", value);
  preferences_fire.end();
}

float loadMaxFire() {
  preferences_fire.begin("fire-settings", true); 
  float value = preferences_fire.getInt("maxFire", 5000); 
  preferences_fire.end();
  return value;
}

void saveMaxTemp(float value) {
  preferences_temperature.begin("temp-settings", false); 
  preferences_temperature.putInt("maxTemp", value);
  preferences_temperature.end();
}

float loadMaxTemp() {
  preferences_temperature.begin("temp-settings", true); 
  float value = preferences_temperature.getInt("maxTemp", 70); 
  preferences_temperature.end();
  return value;
}

void saveMaxHad(float value) {
  preferences_had.begin("had-settings", false); 
  preferences_had.putInt("maxHad", value);
  preferences_had.end();
}

float loadMaxHad() {
  preferences_had.begin("had-settings", true); 
  float value = preferences_had.getInt("maxHad", 70); 
  preferences_had.end();
  return value;
}

void saveMaxServo(int value) {
  preferences_servo.begin("servo-settings", false); 
  preferences_servo.putInt("maxServo", value);
  preferences_servo.end();
}

int loadMaxServo() {
  preferences_servo.begin("servo-settings", true); 
  int value = preferences_servo.getInt("maxServo", 70); 
  preferences_servo.end();
  return value;
}

const int buzzerPin = 27;
const int lightsPin = 26;
const int pin_analog_flame = 32;

const char* ssid = "narzo 50A";
const char* password =  "m7ivjj7c";

const char* id = "EIto";
const char* serverUrl = "https://3piucp-194-87-191-168.ru.tuna.am";

unsigned long lastSendRadio = 0;
unsigned long lastSendTemp = 0;
unsigned long lastBuzzerTime = 0;
unsigned long lastFlameTime = 0;
unsigned long getQuest = 0;
unsigned long getValues = 0;
float fire, temp, had;
int curServo;
bool buzzerState = false;
int number = 1;
byte tries = 10;

Servo servo;

DHT dht(DHTPIN, DHTTYPE);
Alert alert = Alert::NONE_ALERT;


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();
  servo.attach(13);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  pinMode(pin_analog_flame, INPUT);
  float maxFire = loadMaxFire();
  float maxTemp = loadMaxTemp();
  float maxHad = loadMaxTemp();
  int maxServo = loadMaxServo();

  fire = maxFire;
  temp = maxTemp;
  had = maxHad;
  curServo = maxServo;

  Serial.println(String(fire) + " " + String(temp) + " " + String(had));

  while (--tries && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  if (WiFi.status() != WL_CONNECTED)  {
    Serial.println("Non Connecting to WiFi..");
  }
  else  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (!rfdriver.init()) {
    Serial.println("RF init failed");
  } else {
    Serial.println("RF init OK");
  }
  servo.write(curServo);
  Serial.println("ESP32 Ready");
}

void loop() {  
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if (rfdriver.recv(buf, &buflen)) { 
    buf[buflen] = 0;
    Serial.print("Received: ");
    Serial.println((char*)buf);
  }
  if (millis() - getValues > 20000){
    postServo();
    getValues = millis();
  }
  if (millis() - getQuest > 15000){
    getServerData();
    getQuest = millis();
  }

  // Температура каждые 2 секунды
  if (millis() - lastSendTemp > 2000) {
    // считывание данных температуры и влажности
    float h = dht.readHumidity();
    // температура в Цельсиях:
    float t = dht.readTemperature();
    Serial.print("Humidity: ");  //  "Влажность: "
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");  //  "Температура: "
    Serial.print(t);
    Serial.print(" *C ");
    // float ht = t + 10000;
    // float hh = h + 20000;
    // char buf1[16];
    // char buf2[16];
    // snprintf(buf1, sizeof(buf1), "%f", hh);
    // rfdriver.send((uint8_t*)buf1, strlen(buf1));
    // rfdriver.waitPacketSent();
    // snprintf(buf2, sizeof(buf2), "%f", ht);
    // rfdriver.send((uint8_t*)buf2, strlen(buf2));
    // rfdriver.waitPacketSent();
    if (!isnan(t)) {
      if (t > temp) {
        char alertMsg[60];
        sprintf(alertMsg, "High temperature detected: %.1f°C", t);
        sendAlertData(AlertType::TEMPERATURE, alertMsg, "warning");
        alert = Alert::SOUND;
      }
      else {
        sendSensorData(SensorType::TEMPERATURE, t, "C");
      }
    }
    if (!isnan(h)) {
      if (h > had) {
        char alertMsg[60];
        sprintf(alertMsg, "High humidity detected");
        sendAlertData(AlertType::HUMIDITY, alertMsg, "warning");
        alert = Alert::LIGHT;
      }
      else {
        sendSensorData(SensorType::HUMIDITY, h, "H");
      }
      lastSendTemp = millis();
    }
  }
  // Проверка пламени раз в секунду
  if (millis() - lastFlameTime > 1000) {
    float Analog = analogRead (pin_analog_flame);
    Serial.println("Значение аналогового сигнала огня: "); 
    Serial.print(Analog);
    if (Analog < fire){
      sendSensorData(SensorType::FIRE, Analog, "F");
    } else {
      char* alertMsg = "FIRE!";
      sendAlertData(AlertType::MOTION, alertMsg, "alert");
      alert = Alert::SLIGHT;
    }
    char buf[16];
    float hAnalog = Analog + 30000;
    // snprintf(buf, sizeof(buf), "%f", hAnalog);
    // rfdriver.send((uint8_t*)buf, strlen(buf));
    // rfdriver.waitPacketSent();
    lastFlameTime = millis();
  }
  // Пищалка и светодиод (включаем на 1 секунду каждые 5 секунд)
  if (alert == Alert::LIGHT) {
    digitalWrite(lightsPin, HIGH);
  } 
  else if (alert == Alert::SOUND){
    tone(buzzerPin, 1500);
  }
  else if (alert == Alert::SLIGHT) {
    digitalWrite(lightsPin, HIGH);
    tone(buzzerPin, 1500);
  }
  else {
    noTone(buzzerPin);
    digitalWrite(lightsPin, LOW);
  }

  delay(10);
}

void postServo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  HTTPClient http;
  String fullUrl = String(serverUrl) + "/api/v1/devices/" + id + "/values";

  http.begin(fullUrl);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["device_id"] = id;
  doc["temperature_limit"] = temp;
  doc["humidity_limit"] = had;
  doc["fire_limit"] = fire;
  doc["servo_position"] = curServo;
      
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("Sending POST request to: " + fullUrl);
  Serial.println("JSON: " + jsonString);
  
  int httpResponseCode = http.POST(jsonString);
      
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
      
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
    if (httpResponseCode == 200) {
    }
  } else {
    Serial.print("Error in HTTP request: ");
    Serial.println(httpResponseCode);
  }
      
  http.end();
}

void getServerData(){
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String fullUrl = String(serverUrl) + "/api/v1/device/commands/" + id + "/pending";

    http.begin(fullUrl);

    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.GET();

    if(httpResponseCode == 200){
      String payload = http.getString();
      Serial.println("Response: " + payload);
      
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.is<JsonArray>() && doc.size() > 0) {
          for (JsonObject command : doc.as<JsonArray>()) {
            String device_id = command["device_id"];
            String action = command["action"];
            String status = command["status"];
            String par = command["value"];
            String command_id = command["id"];
            String created_at = command["created_at"];
            
            Serial.println("Command received:");
            Serial.println("  ID: " + command_id);
            Serial.println("  Device: " + device_id);
            Serial.println("  Action: " + action);
            Serial.println("  Status: " + status);
            Serial.println("  Created: " + created_at);
            if (action == "toggle_alert") {
              handleToggleAlert();
            }
            else if (action == "set_temerature_limit" && par){
              float param = par.toFloat();
              handleSetTemp(param);
              markCommandAsCompleted(command_id);
            }
            else if (action == "set_fire_limit" && par){
              float param = par.toFloat();
              handleSetFire(param);
              markCommandAsCompleted(command_id);
            }
            else if (action == "set_humidity_limit" && par){
              float param = par.toFloat();
              handleSetHad(param);
              markCommandAsCompleted(command_id);
            }
            else if (action == "set_servo_position" && par){
              float param = par.toFloat();
              handleServo(param);
              markCommandAsCompleted(command_id);
            }
          }
        } else {
          Serial.println("No pending commands");
        }
      } else {
        Serial.println("JSON parsing failed: " + String(error.c_str()));
      }
      
    } else if (httpResponseCode == 422) {
      Serial.println("Validation Error");
      String errorPayload = http.getString();
      Serial.println("Error details: " + errorPayload);
    } else {
      Serial.println("HTTP Error: " + String(httpResponseCode));
      String errorPayload = http.getString();
      Serial.println("Error response: " + errorPayload);
    }
    http.end();
    
  } else {
    Serial.println("WiFi not connected");
  }
}

void handleToggleAlert() {
  if (alert == Alert::NONE_ALERT){
    alert = Alert::SLIGHT;
  }
  else{
    alert = Alert::NONE_ALERT;
  }
  return;
}

void handleServo(float param) {
  curServo = (int)param;
  servo.write(curServo);
  saveMaxServo((int)param);
  return;
}

void handleSetTemp(float param) {
  saveMaxTemp(param);
  Serial.println("temp " + String(param));
  temp = param;
  saveMaxTemp(param);
  alert = Alert::NONE_ALERT;
  return;
}

void handleSetFire(float param){
  fire = param;
  Serial.println("fire " + String(param));
  saveMaxFire(param);
  alert = Alert::NONE_ALERT;
  return;
}

void handleSetHad(float param){
  had = param;
  Serial.println("had " + String(param));
  saveMaxHad(param);
  alert = Alert::NONE_ALERT;
  return;
}

void markCommandAsCompleted(String commandId) {
  HTTPClient http;
  DynamicJsonDocument doc(1024);
  String completeUrl = String(serverUrl) + "/api/v1/device/commands/status";
  http.begin(completeUrl);
  http.addHeader("Content-Type", "application/json");
  doc["device_id"] = id;
  doc["command_id"] = commandId;
  doc["new_status"] = "finished";
  String jsonString;
  serializeJson(doc, jsonString);
  int httpCode = http.PUT(jsonString);
  
  if (httpCode == 200) {
    Serial.println("Command marked as completed");
  } else {
    Serial.println("Failed to mark command as completed: " + String(httpCode));
  }
  http.end();
}

void sendSensorData(SensorType type, float value, const char* unit) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String fullUrl = String(serverUrl) + "/api/v1/devices/" + id +"/readings";
        
        http.begin(fullUrl);
        http.addHeader("Content-Type", "application/json");
        
        // Создаем JSON объект
        DynamicJsonDocument doc(512);
        doc["device_id"] = id;
        doc["sensor_type"] = getSensorTypeName(type);
        doc["value"] = value;
        doc["unit"] = unit;
        
        String jsonString;
        serializeJson(doc, jsonString);
        int httpResponseCode = http.POST(jsonString);
        
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.print("Error in HTTP request: ");
            Serial.println(httpResponseCode);
        }
        
        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}


void sendAlertData(AlertType alert_type, const char* message, const char* severity) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String fullUrl = String(serverUrl) + "/api/v1/alerts/";
        
        http.begin(fullUrl);
        http.addHeader("Content-Type", "application/json");
        
        // Создаем JSON объект
        DynamicJsonDocument doc(512);
        doc["device_id"] = id;
        doc["message"] = message;
        doc["severity"] = severity;
        doc["alert_type"] = getAlertMessage(alert_type);
        
        String jsonString;
        serializeJson(doc, jsonString);
        int httpResponseCode = http.POST(jsonString);
        
        Serial.print("Sending alert: ");
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.print("Error in HTTP request: ");
            Serial.println(httpResponseCode);
        }
        
        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}