#include <RH_ASK.h>
#include <SPI.h>

#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 
#define LCD_RESET A4 

#include <SPI.h>
#include "Adafruit_GFX.h"
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft; 

#define BLACK   0x0000
#define DARKGRAY 0x4208
#define BLUE    0x001F
#define LIGHTBLUE 0x051F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x8410

#define NUM_DEVICES 5
#define ROW_HEIGHT 38
#define STATUS_HEIGHT 28

int currentPage = 0;
int selectedRow = 0;
int lastSelectedRow = -1;
int i = 0;

#define JOY_VRX A5
#define JOY_VRY A4
#define BTN_SELECT 4

RH_ASK rfdriver(4000, 3, 0, 0);

unsigned long lastMove = 0;
const unsigned long MOVE_DELAY = 200;
unsigned long SendTemp = 0;

// ---------------------
struct Device {
  const char* name;
  const char* location;
  const char* status;
};
Device devices[NUM_DEVICES] = {
  {"Climate", "Main Room", "22°C"},
  {"Lock", "Front Door", "ON"},
  {"Lights", "Living Room", "70%"},
  {"Music", "Kitchen", "Playing ♫"},
  {"Camera", "Backyard", "Active"}
};

const unsigned long MAX_WAIT_TIME = 5000;

// ---------------------
void setup() {
  Serial.begin(9600);

  if (!rfdriver.init()) {
    Serial.println("RF init failed");
  } else {
    Serial.println("RF init OK");
  }

  Serial.println("Arduino Uno Receiver ready");
  Serial.println("Waiting for numbers from ESP32...");

  // uint16_t ID = tft.readID();
  // if (ID == 0xD3D3) ID = 0x9481;
  // tft.begin(ID);
  // tft.setRotation(1);
  // tft.fillScreen(BLACK);

  // showSplashScreen();

  // drawHeader();
  // drawTable();
}

void loop() {
  // readJoystick();
  // readButton();
  /*
  
  unsigned long startTime = millis();
  if (millis() - SendTemp > 10000) {
    i = 1 - i;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", i);
    rfdriverRx.send((uint8_t*)buf, strlen(buf));
    rfdriverRx.waitPacketSent(1500);
    SendTemp = millis();
  }

  */
  static int number = 1;
  static unsigned long lastSend = 0;
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if (rfdriver.recv(buf, &buflen)) { // блокирует короткий момент при приеме
      buf[buflen] = 0; // терминируем строку
      float receivedValue = atof((char*)buf);
      int currentType = (int)receivedValue / 10000;
      float currentValue = receivedValue - (currentType * 10000);
      if (currentType == 2)
        Serial.println("Текущая влажность: " + String(currentValue));
      else if (currentType == 1)
        Serial.println("Текущая температура: " + String(currentValue));
      else if (currentType == 3)
        Serial.println("Огонь: " + String(currentValue));
    }
}

// =========================
// UI ELEMENTS
// =========================
void drawHeader() {
  tft.fillRect(0, 0, tft.width(), STATUS_HEIGHT, DARKGRAY);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.print("Smart Control");
  drawBattery(250, 5, 40, 18, 72);
}

void drawBattery(int x, int y, int w, int h, int percent) {
  tft.drawRect(x, y, w, h, WHITE);
  tft.fillRect(x + w, y + h/4, 4, h/2, WHITE);
  int fill = (w - 2) * percent / 100;
  tft.fillRect(x + 1, y + 1, fill, h - 2, GREEN);
}

// =========================
// TABLE SCREEN
// =========================
void drawTable() {
  currentPage = 0;
  fadeOut();
  drawHeader();

  for (int i = 0; i < NUM_DEVICES; i++) {
    int y = STATUS_HEIGHT + 5 + i * ROW_HEIGHT;
    uint16_t color = (i == selectedRow) ? GRAY : BLACK;
    tft.fillRect(0, y, tft.width(), ROW_HEIGHT - 1, color);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, y + 8);
    tft.print(devices[i].name);

    tft.setCursor(130, y + 8);
    // tft.setTextColor(LIGHTBLUE);
    tft.print(devices[i].location);

    tft.setCursor(250, y + 8);
    // tft.setTextColor(YELLOW);
    tft.print(devices[i].status);
  }
}

// =========================
// DEVICE PAGE
// =========================
void drawPage() {
  currentPage = 1;
  fadeOut();
  drawHeader();

//   tft.setTextColor(LIGHTBLUE);
  tft.setTextSize(3);
  tft.setCursor(20, 60);
  tft.print(devices[selectedRow].name);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(20, 110);
  tft.print("Location:");
//   tft.setTextColor(YELLOW);
  tft.setCursor(150, 110);
  tft.print(devices[selectedRow].location);
  tft.setTextColor(WHITE);
  tft.setCursor(20, 150);
  tft.print("Status:");
//   tft.setTextColor(GREEN);
  tft.setCursor(150, 150);
  tft.print(devices[selectedRow].status);
}

// =========================
// ANIMATION: Fade Out
// =========================
void fadeOut() {
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(BLACK);
    delay(20);
  }
}

// =========================
// INPUT HANDLERS
// =========================
void readJoystick() {
  int y = analogRead(JOY_VRX);
  if (millis() - lastMove > MOVE_DELAY) {
    if (currentPage == 0) {
      if (y < 400 && selectedRow > 0) { selectedRow--; drawTable(); lastMove = millis(); }
      else if (y > 600 && selectedRow < NUM_DEVICES - 1) { selectedRow++; drawTable(); lastMove = millis(); }
    }
  }
}

void readButton() {
  static unsigned long lastPress = 0;
  if (digitalRead(BTN_SELECT) == LOW && millis() - lastPress > 250) {
    lastPress = millis();
    if (currentPage == 0) drawPage();
    else drawTable();
    currentPage = 1 - currentPage;
  }
}

// экранчик
void showSplashScreen() {
  tft.fillScreen(BLACK);

  // Цвета и шрифты
  tft.setTextSize(3);
  tft.setTextColor(CYAN);

  String title = "Smart Control";
  int x = (tft.width() - title.length() * 18) / 2; // центрирование по горизонтали
  int y = 100;

  // Эффект "появления" букв
  for (int i = 0; i < title.length(); i++) {
    tft.setCursor(x + i * 18, y);
    tft.print(title[i]);
    delay(100);
  }

  // Легкое мигание после появления
  delay(200);
  for (int i = 0; i < 2; i++) {
    tft.setTextColor(BLACK);
    tft.setCursor(x, y);
    tft.print(title);
    delay(150);
    tft.setTextColor(CYAN);
    tft.setCursor(x, y);
    tft.print(title);
    delay(150);
  }

  // Подпись
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor((tft.width() - 9 * 6) / 2, y + 40); // центр "by hardcode"
  tft.print("by Hardcode");

  delay(1000);
  tft.fillScreen(BLACK);
}