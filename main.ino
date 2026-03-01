#include <Wire.h>
#include <M5Unified.h>
#include <WiFi.h>
#include "time.h"
#include "MAX30100_PulseOximeter.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset = 9 * 3600;
const int   dstOffset = 0;
const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int currentMode = 0;
M5Canvas sprite(&M5.Lcd);

PulseOximeter pox;
float heartRate = 0;
bool hrAvailable = false;
unsigned long lastHrUpdate = 0;
unsigned long lastClockUpdate = 0;

void onBeatDetected() {
  Serial.println("Beat!");
}

void poxTask(void* arg) {
  while (true) {
    if (hrAvailable) pox.update();
    vTaskDelay(1);
  }
}

void drawDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  float voltage = M5.Power.getBatteryVoltage();
  int battery = 0;
  if (voltage >= 4.2) battery = 100;
  else if (voltage <= 3.2) battery = 0;
  else battery = (int)((voltage - 3.2) / 1.0 * 100);
  sprite.fillSprite(BLACK);
  sprite.setTextSize(2);
  sprite.setTextColor(WHITE);
  sprite.setCursor(10, 18);
  sprite.printf("%02d/%02d %s %02d:%02d:%02d",
    timeinfo.tm_mon + 1, timeinfo.tm_mday,
    weekdays[timeinfo.tm_wday],
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  if (battery >= 50) sprite.setTextColor(0x07E0);
  else if (battery >= 20) sprite.setTextColor(0xFD20);
  else sprite.setTextColor(RED);
  sprite.setCursor(255, 18);
  sprite.printf("%d%%", battery);
  sprite.pushSprite(0, 0);
}

void drawExit() {
  M5.Lcd.fillRect(20, 210, 280, 30, DARKGREY);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(120, 215);
  M5.Lcd.println("EXIT");
}

void drawMenu() {
  M5.Lcd.fillScreen(BLACK);
  drawDateTime();
  M5.Lcd.fillRect(5, 50, 150, 88, RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(20, 85);
  M5.Lcd.println("TORII");
  M5.Lcd.fillRect(165, 50, 150, 88, 0xF800);
  M5.Lcd.setCursor(170, 85);
  M5.Lcd.println("HEARTRATE");
}

void drawTorii() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillRect(90,  60, 15, 120, RED);
  M5.Lcd.fillRect(215, 60, 15, 120, RED);
  M5.Lcd.fillRect(60,  50, 200, 15, RED);
  M5.Lcd.fillRect(50,  60, 20,  10, RED);
  M5.Lcd.fillRect(250, 60, 20,  10, RED);
  M5.Lcd.fillRect(80,  80, 160, 12, RED);
  M5.Lcd.fillRect(85, 120, 150,  8, RED);
  drawExit();
}

void drawHeartrateBase() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(60, 60);
  M5.Lcd.println("Heart Rate");
  M5.Lcd.setTextSize(8);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(40, 110);
  M5.Lcd.println("--");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(110, 185);
  M5.Lcd.println("bpm");
  drawExit();
}

void updateHeartrateValues() {
  M5.Lcd.fillRect(40, 110, 240, 70, BLACK);
  M5.Lcd.setTextSize(8);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(40, 110);
  M5.Lcd.printf("%.0f", heartRate);
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = true;
  M5.begin(cfg);
  Serial.begin(115200);
  delay(500);

  Wire.begin(32, 33);
  delay(100);

  if (pox.begin(PULSEOXIMETER_DEBUGGINGMODE_NONE)) {
    hrAvailable = true;
    pox.setOnBeatDetectedCallback(onBeatDetected);
    Serial.println("MAX30100 OK");
  } else {
    Serial.println("MAX30100 FAILED");
  }

  xTaskCreatePinnedToCore(poxTask, "poxTask", 4096, NULL, 1, NULL, 0);

  sprite.createSprite(320, 40);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Connecting WiFi...");

  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) { delay(500); retry++; }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset, dstOffset, ntpServer);
    struct tm timeinfo;
    int ntpRetry = 0;
    while (!getLocalTime(&timeinfo) && ntpRetry < 10) { delay(500); ntpRetry++; }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  drawMenu();
}

void loop() {
  M5.update();

  if (M5.Touch.getCount() > 0) {
    auto t = M5.Touch.getDetail(0);
    if (t.wasReleased()) {
      int tx = t.x;
      int ty = t.y;
      if (currentMode == 0) {
        if (tx < 160 && ty >= 50 && ty <= 138) {
          currentMode = 1; drawTorii();
        } else if (tx >= 160 && ty >= 50 && ty <= 138) {
          currentMode = 2; drawHeartrateBase();
        }
      } else if (currentMode == 1) {
        if (ty >= 210) { currentMode = 0; drawMenu(); }
      } else if (currentMode == 2) {
        if (ty >= 210) { currentMode = 0; drawMenu(); }
      }
    }
  }

  if (currentMode == 0) {
    if (millis() - lastClockUpdate > 1000) {
      lastClockUpdate = millis();
      drawDateTime();
    }
  }

  if (currentMode == 2) {
    if (millis() - lastHrUpdate > 1000) {
      lastHrUpdate = millis();
      if (hrAvailable) {
        heartRate = pox.getHeartRate();
        Serial.printf("HR:%.0f\n", heartRate);
      }
      updateHeartrateValues();
    }
  }
}
