#include <M5Unified.h>
#include <WiFi.h>
#include "time.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset = 9 * 3600;
const int   dstOffset = 0;
const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int currentMode = 0;
bool tapped = false;

M5Canvas sprite(&M5.Lcd);

void drawDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  sprite.fillSprite(BLACK);
  sprite.setTextSize(2);
  sprite.setTextColor(WHITE);

  sprite.setCursor(10, 18);
  sprite.printf("%02d/%02d %s   %02d:%02d:%02d",
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    weekdays[timeinfo.tm_wday],
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec);

  sprite.pushSprite(0, 0);
}

void drawMenu() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);

  drawDateTime();

  M5.Lcd.fillRect(20, 50, 280, 45, BLUE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(90, 62);
  M5.Lcd.println("1. TAP ME");

  M5.Lcd.fillRect(20, 105, 280, 45, RED);
  M5.Lcd.setCursor(80, 117);
  M5.Lcd.println("2. TORII");

  M5.Lcd.fillRect(20, 160, 280, 45, 0x07E0);
  M5.Lcd.setCursor(60, 172);
  M5.Lcd.println("3. CAPYBARA");
}

void drawExit() {
  M5.Lcd.fillRect(20, 210, 280, 30, DARKGREY);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(120, 215);
  M5.Lcd.println("EXIT");
}

void drawTapMe() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(60, 90);
  M5.Lcd.println("TAP ME!");
  drawExit();
}

void drawTapped() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(50, 90);
  M5.Lcd.println("TAPPED!");
  drawExit();
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

void drawCapybara() {
  M5.Lcd.fillScreen(BLACK);

  uint16_t brown     = M5.Lcd.color565(160, 110, 60);
  uint16_t darkbrown = M5.Lcd.color565(80, 45, 15);
  uint16_t pink      = M5.Lcd.color565(255, 180, 180);
  uint16_t cream     = M5.Lcd.color565(255, 240, 200);

  // 胴体（大きく丸く）
  M5.Lcd.fillEllipse(175, 155, 100, 65, brown);

  // 頭（大きく丸く）
  M5.Lcd.fillCircle(95, 115, 52, brown);

  // 耳（丸く大きめ）
  M5.Lcd.fillCircle(68, 72, 18, brown);
  M5.Lcd.fillCircle(118, 70, 18, brown);
  M5.Lcd.fillCircle(68, 72, 11, pink);
  M5.Lcd.fillCircle(118, 70, 11, pink);

  // 目（大きくキラキラ）
  M5.Lcd.fillCircle(80, 108, 11, darkbrown);
  M5.Lcd.fillCircle(110, 108, 11, darkbrown);
  M5.Lcd.fillCircle(77, 105, 4, WHITE);
  M5.Lcd.fillCircle(107, 105, 4, WHITE);
  M5.Lcd.fillCircle(75, 103, 2, WHITE);  // 小ハイライト
  M5.Lcd.fillCircle(105, 103, 2, WHITE);

  // ほっぺ
  M5.Lcd.fillCircle(65, 122, 12, pink);
  M5.Lcd.fillCircle(125, 122, 12, pink);

  // 鼻（丸く）
  M5.Lcd.fillEllipse(95, 130, 16, 10, darkbrown);
  M5.Lcd.fillCircle(89, 130, 4, BLACK);
  M5.Lcd.fillCircle(101, 130, 4, BLACK);

  // 口（笑顔）
  M5.Lcd.drawArc(95, 138, 10, 8, 20, 160, cream);

  // 足（丸く短く）
  M5.Lcd.fillEllipse(115, 205, 18, 14, brown);
  M5.Lcd.fillEllipse(148, 205, 18, 14, brown);
  M5.Lcd.fillEllipse(200, 205, 18, 14, brown);
  M5.Lcd.fillEllipse(233, 205, 18, 14, brown);

  // しっぽ（丸く）
  M5.Lcd.fillCircle(268, 148, 13, brown);

  drawExit();
}

void setup() {
  M5.begin();

  sprite.createSprite(320, 55);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Connecting WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(gmtOffset, dstOffset, ntpServer);

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  drawMenu();
}

void loop() {
  M5.update();

  if (currentMode == 0) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
      lastUpdate = millis();
      drawDateTime();
    }
  }

auto t = M5.Touch.getDetail();
if (t.wasReleased()) {
    int ty = t.y;

    if (currentMode == 0) {
      if (ty >= 50 && ty <= 95) {
        currentMode = 1;
        drawTapMe();
      } else if (ty >= 105 && ty <= 150) {
        currentMode = 2;
        drawTorii();
      } else if (ty >= 160 && ty <= 205) {
        currentMode = 3;
        drawCapybara();
      }

    } else if (currentMode == 1) {
      if (ty >= 210) {
        currentMode = 0;
        tapped = false;
        drawMenu();
      } else {
        if (tapped) {
          drawTapMe();
          tapped = false;
        } else {
          drawTapped();
          tapped = true;
        }
      }

    } else if (currentMode == 2) {
      if (ty >= 210) {
        currentMode = 0;
        drawMenu();
      }

    } else if (currentMode == 3) {
      if (ty >= 210) {
        currentMode = 0;
        drawMenu();
      }
    }

    delay(300);
  }
}
