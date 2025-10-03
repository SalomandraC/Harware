#include <iarduino_RF433_Transmitter.h>

iarduino_RF433_Transmitter radioTX(2);

void setup() {
  Serial.begin(115200);
  radioTX.begin(1000);                   
  radioTX.openWritingPipe(5);            
  
  Serial.println("ESP32 Transmitter ready");
  Serial.println("Sending numbers 1-10...");
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
}