#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>

iarduino_RF433_Transmitter radioTX(2);  // Передатчик на пине D2
iarduino_RF433_Receiver radioRX(4);     // Приемник на пине D4

void setup() {
  Serial.begin(115200);
  
  // Инициализация передатчика
  radioTX.begin(1000);                   
  radioTX.openWritingPipe(5);          

  // Инициализация приемника
  radioRX.begin(1000);                   
  radioRX.openReadingPipe(5);
  radioRX.startListening();
  
  Serial.println("ESP32 Transmitter & Receiver ready");
}

void loop() {
  static int number = 1;
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    char data[10];
    sprintf(data, "%d", number);      
    radioTX.write(&data, sizeof(data));
    
    Serial.print("Sent: ");
    Serial.println(number);
    number++;
    if (number > 10) number = 1;
    
    lastSend = millis();
  }
  if (radioRX.available()) {
    char receivedData[10] = "";
    radioRX.read(&receivedData, sizeof(receivedData));
    
    Serial.print("Received: ");
    Serial.println(receivedData);
  }
  delay(10);
}