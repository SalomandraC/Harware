#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>
#include "DHT.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include <RH_ASK.h>
#include <SPI.h>
#define DHTPIN 33
#define DHTTYPE DHT11

RH_ASK rfdriver(2000, 0, 2, 0);


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


String getAlertMessage(AlertType alert) {
    switch (alert) {
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


const int buzzerPin = 27;
const int lightsPin = 26;
const int pin_analog_flame = 32;

const char* ssid = "realme 8";
const char* password =  "einmn6cw";

const char* id = "EIto";
const char* serverUrl = "https://ghlwjg-95-174-102-182.ru.tuna.am";

unsigned long lastSendRadio = 0;
unsigned long lastSendTemp = 0;
unsigned long lastBuzzerTime = 0;
unsigned long lastFlameTime = 0;
bool buzzerState = false;
int number = 1;
byte tries = 10;

DHT dht(DHTPIN, DHTTYPE);
Alert alert = Alert::NONE_ALERT;


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();
  pinMode(buzzerPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  pinMode(pin_analog_flame, INPUT);


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
  Serial.println("ESP32 Ready");
}

void loop() {  
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
    float ht = t + 10000;
    float hh = h + 20000;
    char buf1[16];
    char buf2[16];
    snprintf(buf1, sizeof(buf1), "%f", hh);
    rfdriver.send((uint8_t*)buf1, strlen(buf1));
    rfdriver.waitPacketSent();
    snprintf(buf2, sizeof(buf2), "%f", ht);
    rfdriver.send((uint8_t*)buf2, strlen(buf2));
    rfdriver.waitPacketSent();
    if (!isnan(t)) {
      if (t > 40.0) {
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
      if (h > 80.0) {
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
    if (Analog < 5000){
      sendSensorData(SensorType::FIRE, Analog, "F");
    } else {
      char* alertMsg = "FIRE!";
      sendAlertData(AlertType::MOTION, alertMsg, "alert");
      alert = Alert::SLIGHT;
    }
    char buf[16];
    float hAnalog = Analog + 30000;
    snprintf(buf, sizeof(buf), "%f", hAnalog);
    rfdriver.send((uint8_t*)buf, strlen(buf));
    rfdriver.waitPacketSent();
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

void sendSensorData(SensorType type, float value, const char* unit) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String fullUrl = String(serverUrl) + "/api/v1/devices/{device_id}/readings";
        
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