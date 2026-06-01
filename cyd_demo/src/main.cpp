/**
 * CYD demo – TFT_eSPI (ILI9341_2_DRIVER) + XPT2046 touch.
 * Enaka logika kot Arduino “borders with touch.ino”: rotation 1, invertDisplay(true),
 * cel zaslon 240×320, touch mapiran z 200,3700 / 200,3800.
 */
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

#define PEN_SIZE   5
#define PEN_COLOR  TFT_WHITE

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("CYD demo - TFT_eSPI + touch");

  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);

  uint16_t w = tft.width();
  uint16_t h = tft.height();
  Serial.printf("TFT %d x %d\n", w, h);

  const uint8_t border = 6;
  for (int i = 0; i < border; i++) {
    tft.drawRect(i, i, w - 2*i, h - 2*i, TFT_GREEN);
  }

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("1", border + 10, border + 10);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("2", w - border - 10, border + 10);
  tft.setTextDatum(BL_DATUM);
  tft.drawString("3", border + 10, h - border - 10);
  tft.setTextDatum(BR_DATUM);
  tft.drawString("4", w - border - 10, h - border - 10);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(5);
  tft.drawString("5", w / 2, h / 2);

  tft.drawFastVLine(w / 2, 0, h, TFT_RED);
  tft.drawFastHLine(0, h / 2, w, TFT_YELLOW);

  tft.setTextSize(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Dotikaj se za risanje", w/2, h - 30);

  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(1);

  Serial.println("Pripravljeno - dotikaj se zaslona");
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    if (p.z < 300) {
      delay(10);
      return;
    }
    int16_t x = map(p.x, 200, 3700, 0, tft.width());
    int16_t y = map(p.y, 200, 3800, 0, tft.height());
    x = constrain(x, 0, tft.width() - 1);
    y = constrain(y, 0, tft.height() - 1);
    tft.fillCircle(x, y, PEN_SIZE / 2, PEN_COLOR);
    Serial.printf("Dotik: x=%d y=%d\n", x, y);
    delay(5);
  }
  delay(10);
}
