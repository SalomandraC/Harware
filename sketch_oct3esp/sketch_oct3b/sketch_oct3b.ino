#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>
#include "DHT.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#define DHTPIN 33
#define DHTTYPE DHT11

iarduino_RF433_Transmitter radioTX(2);
iarduino_RF433_Receiver radioRX(4);

enum class AlertType {
    STUDY = 0,
    ERROR = 1,
    TEMPERATURE = 2,
    HUMIDITY = 3,
    MOTION = 4,
    BATTERY = 5
};

const char* alertTypeStrings[] = {"study", "error", "temperature", "humidity", "motion", "battery"};

enum class SensorType {
    TEMPERATURE = 0,
    HUMIDITY = 1,
    ALERT = 2,
    FIRE = 3
};

const char* sensorTypeStrings[] = {"temperature", "humidity", "alert"};

const int buzzerPin = 27;
const int lightsPin = 26;

const char* ssid = "narzo 50A";
const char* password =  "m7ivjj7c";

const char* id = "EIto";
const char* serverUrl = "https://2g6nw0-194-87-191-168.ru.tuna.am";

unsigned long lastSendRadio = 0;
unsigned long lastSendTemp = 0;
unsigned long lastBuzzerTime = 0;
bool buzzerState = false;
int number = 1;
byte tries = 10;

DHT dht(DHTPIN, DHTTYPE);
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();
  pinMode(buzzerPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  
  radioTX.begin(1000);                   
  radioTX.openWritingPipe(5);          
  radioRX.begin(1000);                   
  radioRX.openReadingPipe(5);
  radioRX.startListening();

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
  
  Serial.println("ESP32 Ready");
}

void loop() {
  // RF передача каждые 2 секунды
  if (millis() - lastSendRadio > 2000) {
    char data[10];
    sprintf(data, "%d", number);      
    radioTX.write(&data, sizeof(data));
    
    Serial.print("Sent: ");
    Serial.println(number);
    number++;
    if (number > 10) number = 1;
    if(number > 4 && number < 7){
      sendSensorData(SensorType::ALERT, 1, "%");
    }
    
    lastSendRadio = millis();
  }
  
  // RF прием
  if (radioRX.available()) {
    char receivedData[10] = "";
    radioRX.read(&receivedData, sizeof(receivedData));
    Serial.println("Received: ");
    Serial.print(receivedData);
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
    if (!isnan(t)) {
      if (t > 40.0) {
        char alertMsg[60];
        sprintf(alertMsg, "High temperature detected: %.1f°C", t);
        sendAlertData(AlertType::TEMPERATURE, alertMsg, "warning");
      }
      else {
        sendSensorData(SensorType::TEMPERATURE, t, "C");
      }
    }
    if (!isnan(h)) {
      sendSensorData(SensorType::HUMIDITY, h, "H");
    }
    lastSendTemp = millis();
  }
  
  
  // Пищалка и светодиод (включаем на 1 секунду каждые 10 секунд)
  if (millis() - lastBuzzerTime > 5000) {
    tone(buzzerPin, 1500);
    digitalWrite(lightsPin, HIGH);
    delay(1000); // Включаем на 1 секунду
    noTone(buzzerPin);
    digitalWrite(lightsPin, LOW);
    lastBuzzerTime = millis();
  }

  delay(10);
}

void sendSensorData(SensorType type, float value, const char* message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String fullUrl = String(serverUrl) + "/api/v1/devices/{device_id}/readings";
        
        http.begin(fullUrl);
        http.addHeader("Content-Type", "application/json");
        
        // Создаем JSON объект
        DynamicJsonDocument doc(512);
        doc["device_id"] = id;
        doc["sensor_type"] = sensorTypeStrings[static_cast<int>(type)];
        doc["value"] = value;
        doc["unit"] = message;
        
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
        doc["alert_type"] = alertTypeStrings[static_cast<int>(alert_type)];
        
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


