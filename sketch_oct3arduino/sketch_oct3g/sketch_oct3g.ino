// Arduino_receiver_RH_ASK.ino
#include <RH_ASK.h>
#include <SPI.h>

RH_ASK rfdriver(2000, 3, 0, 0);

void setup() {
  Serial.begin(9600);
  if (!rfdriver.init()) {
    Serial.println("RF init failed");
  } else {
    Serial.println("RF init OK");
  }
}

void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (rfdriver.recv(buf, &buflen)) { // блокирует короткий момент при приеме
    buf[buflen] = 0; // терминируем строку
    Serial.print("Received: ");
    Serial.println((char*)buf);
  }
}
