#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>

iarduino_RF433_Transmitter radioTX(5);
iarduino_RF433_Receiver radioRX(3);

// Буферы для данных
uint32_t txData = 0;
uint32_t rxData = 0;

void setup() {
  Serial.begin(9600);
  
  // Инициализация RF433
  radioTX.begin(1000);      // Скорость 1000 бит/с
  radioRX.begin(1000);
  
  radioTX.openWritingPipe(5);   // Труба 5
  radioRX.openReadingPipe(6);   // Труба 5
  radioRX.startListening();
  
  Serial.println("Arduino Uno Ready - Communicating with ESP32");
}

void loop() {
  // ПРИЕМ ДАННЫХ ОТ ESP32
  uint8_t pipeNum;
  if (radioRX.available(&pipeNum)) {
    // Читаем данные как число (совместимость с RCSwitch)
    radioRX.read(&rxData, sizeof(rxData));
    
    Serial.print("Arduino ← ESP32: ");
    Serial.println(rxData);
    
    // Реагируем на полученные данные
    if (rxData >= 1000 && rxData <= 1010) {
      Serial.println("  → Counter received from ESP32");
    }
  }
  
  // ПЕРЕДАЧА ДАННЫХ ДЛЯ ESP32
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 4000) {
    // Отправляем числа для совместимости с RCSwitch
    static int arduinoCounter = 5000;
    txData = arduinoCounter;
    
    radioTX.write(&txData, sizeof(txData));
    
    Serial.print("Arduino → ESP32: ");
    Serial.println(arduinoCounter);
    
    arduinoCounter++;
    if (arduinoCounter > 5010) arduinoCounter = 5000;
    lastSend = millis();
  }
  
  delay(200);
}