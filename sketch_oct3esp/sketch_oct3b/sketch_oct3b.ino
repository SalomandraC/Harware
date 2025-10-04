// ESP32_transmitter_RH_ASK.ino
#include <RH_ASK.h>
#include <SPI.h> 

RH_ASK rfdriver(2000, 0, 2, 0);

void setup() {
  Serial.begin(115200);
  delay(100);
  if (!rfdriver.init()) {
    Serial.println("RF init failed");
  } else {
    Serial.println("RF init OK");
  }
}

void loop() {
  unsigned long number = 12345;
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", number);
  rfdriver.send((uint8_t*)buf, strlen(buf));
  rfdriver.waitPacketSent();

  Serial.print("Sent: ");
  Serial.println(buf);
  delay(1000);
}
