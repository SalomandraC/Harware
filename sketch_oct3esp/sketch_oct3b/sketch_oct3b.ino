#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>
#include "DHT.h"
#include "WiFi.h"
#define DHTPIN 33
#define DHTTYPE DHT11

iarduino_RF433_Transmitter radioTX(2);
iarduino_RF433_Receiver radioRX(4);

const int buzzerPin = 27;
const int lightsPin = 26;

const char* ssid = "narzo 50A";
const char* password =  "m7ivjj7c";

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