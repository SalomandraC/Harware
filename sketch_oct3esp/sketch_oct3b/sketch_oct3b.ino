#include <iarduino_RF433_Transmitter.h>
#include <iarduino_RF433_Receiver.h>

iarduino_RF433_Transmitter radioTX(2);
iarduino_RF433_Receiver radioRX(4);

const int buzzerPin = 27;
const int lightsPin = 26;
//const int DHpin = 33;

byte dat[5];
unsigned long lastSendRadio = 0;
unsigned long lastSendTemp = 0;
unsigned long lastBuzzerTime = 0;
bool buzzerState = false;
int number = 1;

void setup() {
  Serial.begin(115200);
  //pinMode(DHpin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(lightsPin, OUTPUT);
  
  radioTX.begin(1000);                   
  radioTX.openWritingPipe(5);          
  radioRX.begin(1000);                   
  radioRX.openReadingPipe(5);
  radioRX.startListening();
  
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
    Serial.print("Received: ");
    Serial.println(receivedData);
  }
  /*
  // Температура каждые 2 секунды
  if (millis() - lastSendTemp > 2000) {
    start_test();
    Serial.print("Humdity = ");
    Serial.print(dat[0], DEC);
    Serial.print('.');
    Serial.print(dat[1], DEC);
    Serial.println('%');
    Serial.print("Temperature = ");
    Serial.print(dat[2], DEC);
    Serial.print('.');
    Serial.print(dat[3], DEC);
    Serial.println('C');
    lastSendTemp = millis();
  }
  */
  
  // Пищалка и светодиод (включаем на 1 секунду каждые 10 секунд)
  if (millis() - lastBuzzerTime > 10000) {
    tone(buzzerPin, 500);
    digitalWrite(lightsPin, HIGH);
    delay(1000); // Включаем на 1 секунду
    noTone(buzzerPin);
    digitalWrite(lightsPin, LOW);
    lastBuzzerTime = millis();
  }
  
  delay(10);
}
/*
byte read_data() {
  byte i = 0;
  byte result = 0;
  for (i = 0; i < 8; i++) {
    while (digitalRead(DHpin) == LOW);
    delayMicroseconds(30);
    if (digitalRead(DHpin) == HIGH)
      result |= (1 << (8 - i));
    while (digitalRead(DHpin) == HIGH);
  }
  return result;
}

void start_test() {
  digitalWrite(DHpin, LOW);
  delay(30);
  digitalWrite(DHpin, HIGH);
  delayMicroseconds(40);
  pinMode(DHpin, INPUT);
  while(digitalRead(DHpin) == HIGH);
  delayMicroseconds(80);
  
  if(digitalRead(DHpin) == LOW)
    delayMicroseconds(80);
  for(int i = 0; i < 5; i++)
    dat[i] = read_data();
  pinMode(DHpin, OUTPUT);
  digitalWrite(DHpin, HIGH);
}
*/
