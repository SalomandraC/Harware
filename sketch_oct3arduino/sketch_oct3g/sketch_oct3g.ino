#include <iarduino_RF433_Receiver.h>

iarduino_RF433_Receiver radioRX(3);     // Приемник на пине 3

void setup() {
  Serial.begin(9600);
  
  // Инициализация приемника
  radioRX.begin(1000);                   // Скорость 1 кбит/сек
  radioRX.openReadingPipe(5);            // Труба №5
  radioRX.startListening();
  
  Serial.println("Arduino Uno Receiver ready");
  //Serial.println("Waiting for numbers from ESP32...");
}

void loop() {
  if (radioRX.available()) {
    char receivedData[10] = "";
    radioRX.read(&receivedData, sizeof(receivedData));
    
    Serial.print("Received: ");
    Serial.println(receivedData);
  }
}