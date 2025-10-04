#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(115200);
  
  // Передатчик на пине 4
  mySwitch.enableTransmit(2);
  // Приемник на пине 5
  mySwitch.enableReceive(4);

  mySwitch.setProtocol(1);          // Протокол 1
  mySwitch.setPulseLength(300);     // Длина импульса
  mySwitch.setRepeatTransmit(5);    // Повторов для надежности
  
  Serial.println("ESP32 Ready - Communicating with Arduino Uno");
}

void loop() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 3000) {
    static int counter = 1000;
    mySwitch.send(counter, 24);     
    
    Serial.print("ESP32 → Arduino: ");
    Serial.println(counter);
    
    counter++;
    if (counter > 1010) counter = 1000;
    lastSend = millis();
  }

  if (mySwitch.available()) {
    unsigned long receivedValue = mySwitch.getReceivedValue();
    
    if (receivedValue != 0) {
      Serial.print("ESP32 ← Arduino: ");
      Serial.println(receivedValue);
      if (receivedValue == 123456) {
        Serial.println("  → Special command: HELLO");
      }
    }
    
    mySwitch.resetAvailable();
  }
  
  delay(100);
}