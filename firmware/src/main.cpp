/**
 * Rastlinjak – ESP32: TFT_eSPI (ILI9341_2), header (ura, FAN, IRR),
 * mreža 6 senzorjev (simulacija), graf ob kliku, orodna vrstica.
 * Zaslon: setRotation(1), invertDisplay(true), (0,0) zgoraj levo.
 */
#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "config.h"

// 1 = ob dotiku izpiši surove x,y,z v Serial (za prilagoditev map/praga)
#define TOUCH_DEBUG 0

// Dimenzije nastavimo v setup() po setRotation(1) → običajno 320×240
int TFT_W = 320;
int TFT_H = 240;

TFT_eSPI tft = TFT_eSPI();
// Enako kot cyd_demo: CS 33, IRQ 36 (delujoča kombinacija)
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
WebServer webServer(WEB_SERVER_PORT);

#define COLOR_BG            0x3186
#define COLOR_HEADER        0xFFFF   // belo ozadje headerja
#define COLOR_HEADER_TEXT   0x4008   // temno modri napisi (ura, naslov)
#define COLOR_HEADER_BORDER 0x0010   // temno modra ločnica
#define COLOR_TEXT          0xFFFF
#define COLOR_GRID          0xCE79
#define COLOR_CELL_BORDER   0x07E0   // zelen rob celic (ikone)
#define COLOR_CELL_FILL     0xAE5F   // zelo nežna modra notranjost ikon
#define COLOR_TOOLBAR       0x2104
#define COLOR_BTN_OFF       0x001F   // modra (FAN/IRR off)
#define COLOR_BTN_ON        0x07E0   // zelena (FAN/IRR on)
#define HEADER_TITLE        "GRENSAN"
#define HEADER_BORDER_H     4       // debelina temno modre črte pod vrstico
#define HEADER_TOTAL_H      (TOOLBAR_H + HEADER_BORDER_H)
#define LONG_PRESS_MS       500     // FAN/IRR reagirata po 0,5 s držanja

float sim_moisture     = 45.0f;
float sim_temp         = 22.3f;
float sim_temp2        = 21.1f;
float sim_temp3        = 20.8f;
float sim_soil_temp    = 19.5f;
float sim_soil_humidity = 52.0f;
bool  relay_fan        = false;
bool  relay_water  = false;

enum Screen { SCREEN_GRID, SCREEN_GRAPH };
Screen currentScreen = SCREEN_GRID;
int    graphSensor   = 0;
uint32_t lastDraw = 0, lastSimUpdate = 0;
int8_t  pendingHeaderBtn = -1;      // 0=FAN, 1=IRR, -1=ni
uint32_t pendingHeaderStart = 0;
bool hasTimeSync = false;
uint32_t syncedAtMs = 0;
uint32_t syncedEpoch = 0;

static void handleWebRoot() {
  static const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>GRENSAN Demo</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 16px; background: #f3f7fb; color: #0d2b55; }
    .card { background: #fff; border-radius: 12px; padding: 14px; margin-bottom: 12px; box-shadow: 0 2px 8px rgba(0,0,0,.08); }
    h1 { margin: 0 0 10px 0; font-size: 22px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .val { font-size: 26px; font-weight: 700; color: #003a8c; }
    .lbl { font-size: 13px; color: #4d6f99; }
    .pill { display: inline-block; padding: 6px 10px; border-radius: 999px; color: #fff; font-weight: 700; font-size: 12px; }
    .on { background: #00a651; }
    .off { background: #1e5aa8; }
  </style>
</head>
<body>
  <h1>GRENSAN Demo (simulacija)</h1>
  <div class="card">
    <div>AP SSID: <b>GRENSAN-DEMO</b></div>
    <div>IP ESP32: <b id="ip">192.168.4.1</b></div>
  </div>
  <div class="card">
    <span class="pill" id="fan">FAN</span>
    <span class="pill" id="irr">IRR</span>
  </div>
  <div class="grid">
    <div class="card"><div class="lbl">Vlaga</div><div class="val" id="moisture">-</div></div>
    <div class="card"><div class="lbl">Temp1</div><div class="val" id="temp1">-</div></div>
    <div class="card"><div class="lbl">Temp2</div><div class="val" id="temp2">-</div></div>
    <div class="card"><div class="lbl">Temp3</div><div class="val" id="temp3">-</div></div>
    <div class="card"><div class="lbl">Soil temp</div><div class="val" id="soilTemp">-</div></div>
    <div class="card"><div class="lbl">Soil hum</div><div class="val" id="soilHum">-</div></div>
  </div>
  <script>
    function setRelay(el, on, name) {
      el.textContent = name + ': ' + (on ? 'ON' : 'OFF');
      el.className = 'pill ' + (on ? 'on' : 'off');
    }
    async function refresh() {
      if (!window._timeSynced) {
        window._timeSynced = true;
        fetch('/api/setTime?ts=' + Math.floor(Date.now() / 1000)).catch(() => {});
      }
      const r = await fetch('/api/status');
      const d = await r.json();
      document.getElementById('ip').textContent = d.ip;
      document.getElementById('moisture').textContent = d.moisture.toFixed(1) + '%';
      document.getElementById('temp1').textContent = d.temp1.toFixed(1) + 'C';
      document.getElementById('temp2').textContent = d.temp2.toFixed(1) + 'C';
      document.getElementById('temp3').textContent = d.temp3.toFixed(1) + 'C';
      document.getElementById('soilTemp').textContent = d.soilTemp.toFixed(1) + 'C';
      document.getElementById('soilHum').textContent = d.soilHum.toFixed(1) + '%';
      setRelay(document.getElementById('fan'), d.relayFan, 'FAN');
      setRelay(document.getElementById('irr'), d.relayWater, 'IRR');
    }
    refresh();
    setInterval(refresh, 1500);
  </script>
</body>
</html>
)HTML";
  webServer.send(200, "text/html; charset=utf-8", PAGE);
}

static void handleWebStatus() {
  String json = "{";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"moisture\":" + String(sim_moisture, 1) + ",";
  json += "\"temp1\":" + String(sim_temp, 1) + ",";
  json += "\"temp2\":" + String(sim_temp2, 1) + ",";
  json += "\"temp3\":" + String(sim_temp3, 1) + ",";
  json += "\"soilTemp\":" + String(sim_soil_temp, 1) + ",";
  json += "\"soilHum\":" + String(sim_soil_humidity, 1) + ",";
  json += "\"relayFan\":" + String(relay_fan ? "true" : "false") + ",";
  json += "\"relayWater\":" + String(relay_water ? "true" : "false");
  json += "}";
  webServer.send(200, "application/json", json);
}

static void handleWebSetTime() {
  if (!webServer.hasArg("ts")) {
    webServer.send(400, "text/plain", "missing ts");
    return;
  }
  uint32_t ts = (uint32_t)webServer.arg("ts").toInt();
  if (ts < 1000000000UL) {
    webServer.send(400, "text/plain", "invalid ts");
    return;
  }
  syncedEpoch = ts;
  syncedAtMs = millis();
  hasTimeSync = true;
  webServer.send(200, "text/plain", "ok");
}

static void startWebServer() {
  WiFi.mode(WIFI_AP);
  bool apOk = WiFi.softAP(WEB_AP_SSID, WEB_AP_PASSWORD);
  if (apOk) {
    Serial.printf("AP ready: %s\n", WEB_AP_SSID);
    Serial.print("Open http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("AP start failed");
  }

  webServer.on("/", handleWebRoot);
  webServer.on("/api/status", handleWebStatus);
  webServer.on("/api/setTime", handleWebSetTime);
  webServer.begin();
  Serial.printf("Web server started on port %d\n", WEB_SERVER_PORT);
}

static void getTimeString(char* buf, size_t len) {
  unsigned long s;
  if (hasTimeSync) {
    s = syncedEpoch + ((millis() - syncedAtMs) / 1000UL);
  } else {
    s = millis() / 1000UL;
  }
  snprintf(buf, len, "%02lu:%02lu", (s / 3600) % 24, (s / 60) % 60);
}

// Zona ure levo, naslov na sredini, FAN/IRR ovalna gumba desno
#define HEADER_TIME_W   58
#define HEADER_BTN_W    48
#define HEADER_BTN_H    26
#define HEADER_BTN_GAP  8

static void drawHeaderOvalButton(int x, int y, int w, int h, bool on, const char* label) {
  uint16_t bg = on ? COLOR_BTN_ON : COLOR_BTN_OFF;
  int r = h / 2;
  tft.fillRoundRect(x, y, w, h, r, bg);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  int tw = 6 * (int)strlen(label);
  tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
  tft.print(label);
}

void drawHeader() {
  tft.fillRect(0, 0, TFT_W, TOOLBAR_H, COLOR_HEADER);
  tft.fillRect(0, TOOLBAR_H, TFT_W, HEADER_BORDER_H, COLOR_HEADER_BORDER);
  tft.setTextColor(COLOR_HEADER_TEXT);
  tft.setTextSize(2);
  char timeBuf[8];
  getTimeString(timeBuf, sizeof(timeBuf));
  tft.setCursor(6, (TOOLBAR_H - 16) / 2);
  tft.print(timeBuf);

  int cx = TFT_W / 2;
  tft.setTextSize(2);
  int titleW = 12 * (int)strlen(HEADER_TITLE);
  tft.setCursor(cx - titleW / 2 - 5, (TOOLBAR_H - 16) / 2);
  tft.print(HEADER_TITLE);

  int by = (TOOLBAR_H - HEADER_BTN_H) / 2;
  int rx = TFT_W - (HEADER_BTN_W * 2 + HEADER_BTN_GAP + 10);
  drawHeaderOvalButton(rx, by, HEADER_BTN_W, HEADER_BTN_H, relay_fan, "FAN");
  drawHeaderOvalButton(rx + HEADER_BTN_W + HEADER_BTN_GAP, by, HEADER_BTN_W, HEADER_BTN_H, relay_water, "IRR");
}

// Vrne 0 = FAN, 1 = IRR, -1 = ni v gumbu (tx,ty v koordinatah zaslona)
static int hitHeaderButton(int tx, int ty) {
  if (ty < 0 || ty >= TOOLBAR_H) return -1;
  int by = (TOOLBAR_H - HEADER_BTN_H) / 2;
  int rx = TFT_W - (HEADER_BTN_W * 2 + HEADER_BTN_GAP + 10);
  if (ty < by || ty >= by + HEADER_BTN_H) return -1;
  if (tx >= rx && tx < rx + HEADER_BTN_W) return 0;
  if (tx >= rx + HEADER_BTN_W + HEADER_BTN_GAP && tx < rx + HEADER_BTN_W * 2 + HEADER_BTN_GAP) return 1;
  return -1;
}

void updateHeaderTime() {
  tft.fillRect(0, 0, HEADER_TIME_W + 4, TOOLBAR_H, COLOR_HEADER);
  tft.setTextColor(COLOR_HEADER_TEXT);
  tft.setTextSize(2);
  char timeBuf[8];
  getTimeString(timeBuf, sizeof(timeBuf));
  tft.setCursor(6, (TOOLBAR_H - 16) / 2);
  tft.print(timeBuf);
}

void drawToolbar() {
  int ly = TFT_H - TOOLBAR_H;
  tft.fillRect(0, ly, TFT_W, TOOLBAR_H, COLOR_TOOLBAR);
}

void drawGrid() {
  int top = HEADER_TOTAL_H;
  int bottom = TFT_H - TOOLBAR_H;
  int areaH = bottom - top;
  const int gap = 4;
  const int numCols = 3;
  int cellW = (TFT_W - (numCols + 1) * gap) / numCols;
  int cellH = (areaH - 3 * gap) / 2;

  tft.fillRect(0, top, TFT_W, areaH, COLOR_BG);
  const int radius = 18;

  struct Cell { const char* label; float value; const char* unit; };
  Cell cells[] = {
    { "Vlaga",      sim_moisture,      "%" },
    { "Temp1",      sim_temp,          "C" },
    { "Temp2",      sim_temp2,         "C" },
    { "Temp3",      sim_temp3,         "C" },
    { "Soil temp",  sim_soil_temp,     "C" },
    { "Soil hum",   sim_soil_humidity, "%" },
  };

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int ox = gap + col * (cellW + gap);
    int oy = top + gap + row * (cellH + gap);
    int cw = cellW;
    int ch = cellH;
    tft.fillRoundRect(ox, oy, cw, ch, radius, COLOR_CELL_FILL);
    tft.drawRoundRect(ox, oy, cw, ch, radius, COLOR_CELL_BORDER);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    int labelW = (int)strlen(cells[i].label) * 6;
    tft.setCursor(ox + (cw - labelW) / 2, oy + 4);
    tft.print(cells[i].label);
    tft.setTextSize(2);
    char valBuf[16];
    snprintf(valBuf, sizeof(valBuf), "%.1f%s", cells[i].value, cells[i].unit);
    int valW = (int)strlen(valBuf) * 12;
    int valH = 16;
    tft.setCursor(ox + (cw - valW) / 2, oy + (ch - valH) / 2);
    tft.print(valBuf);
  }
}

// Posodobi samo vrednosti v celicah (brez risanja ozadja in okvirjev) – zmanjša flickering
void updateGridValues() {
  int top = HEADER_TOTAL_H;
  int bottom = TFT_H - TOOLBAR_H;
  int areaH = bottom - top;
  const int gap = 4;
  const int numCols = 3;
  int cellW = (TFT_W - (numCols + 1) * gap) / numCols;
  int cellH = (areaH - 3 * gap) / 2;
  const int valH = 16;

  struct Cell { const char* label; float value; const char* unit; };
  Cell cells[] = {
    { "Vlaga",      sim_moisture,      "%" },
    { "Temp1",      sim_temp,          "C" },
    { "Temp2",      sim_temp2,         "C" },
    { "Temp3",      sim_temp3,         "C" },
    { "Soil temp",  sim_soil_temp,     "C" },
    { "Soil hum",   sim_soil_humidity, "%" },
  };

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int ox = gap + col * (cellW + gap);
    int oy = top + gap + row * (cellH + gap);
    int cw = cellW;
    int ch = cellH;
    int valY = oy + (ch - valH) / 2;
    tft.fillRect(ox, valY, cw, valH, COLOR_CELL_FILL);
    char valBuf[16];
    snprintf(valBuf, sizeof(valBuf), "%.1f%s", cells[i].value, cells[i].unit);
    int valW = (int)strlen(valBuf) * 12;
    tft.setCursor(ox + (cw - valW) / 2, valY);
    tft.print(valBuf);
  }
}

void drawGraphScreen() {
  tft.fillScreen(COLOR_BG);
  drawHeader();
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(10, HEADER_TOTAL_H + 10);
  const char* names[] = { "Vlaga", "Temp1", "Temp2", "Temp3", "Soil temp", "Soil hum" };
  tft.printf("Graf: %s", names[graphSensor]);

  int gx = 10, gy = HEADER_TOTAL_H + 50, gw = TFT_W - 20, gh = TFT_H - HEADER_TOTAL_H - TOOLBAR_H - 70;
  tft.drawRect(gx, gy, gw, gh, COLOR_GRID);
  float vals[] = { sim_moisture, sim_temp, sim_temp2, sim_temp3, sim_soil_temp, sim_soil_humidity };
  float v = graphSensor >= 0 && graphSensor < 6 ? vals[graphSensor] : 0;
  float pct = 0;
  if (graphSensor == 0 || graphSensor == 5) {
    pct = v;
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
  } else {
    pct = (v - 10.0f) / 30.0f * 100.0f;
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
  }
  int barH = (int)((float)gh * (pct / 100.0f));
  if (barH > gh) barH = gh;
  tft.fillRect(gx + 2, gy + gh - barH, 20, barH, COLOR_GRID);

  tft.setCursor(10, TFT_H - TOOLBAR_H - 25);
  tft.print("Klik = nazaj");
  drawToolbar();
}

void screenGrid() {
  currentScreen = SCREEN_GRID;
  tft.fillScreen(COLOR_BG);
  drawHeader();
  drawGrid();
  drawToolbar();
}

void screenGraph(int sensorIndex) {
  graphSensor = sensorIndex;
  currentScreen = SCREEN_GRAPH;
  drawGraphScreen();
}

int hitCell(int tx, int ty) {
  int top = HEADER_TOTAL_H;
  int bottom = TFT_H - TOOLBAR_H;
  const int gap = 4;
  const int numCols = 3;
  int cellW = (TFT_W - (numCols + 1) * gap) / numCols;
  int cellH = (bottom - top - 3 * gap) / 2;
  if (ty < top || ty >= bottom) return -1;
  int y0 = top + gap;
  int row = (ty < y0 + cellH) ? 0 : 1;
  int x0 = gap;
  for (int c = 0; c < 3; c++) {
    if (tx >= x0 && tx < x0 + cellW) return row * 3 + c;
    x0 += cellW + gap;
  }
  return -1;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  TFT_W = tft.width();
  TFT_H = tft.height();
  tft.fillScreen(COLOR_BG);
  Serial.printf("TFT %d x %d\n", TFT_W, TFT_H);

  SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin();
  touch.setRotation(1);

  pinMode(RELAY_FAN_PIN, OUTPUT);
  pinMode(RELAY_WATER_PIN, OUTPUT);
  digitalWrite(RELAY_FAN_PIN, relay_fan ? RELAY_ON : !RELAY_ON);
  digitalWrite(RELAY_WATER_PIN, relay_water ? RELAY_ON : !RELAY_ON);

  startWebServer();
  screenGrid();
  lastDraw = lastSimUpdate = millis();
}

void loop() {
  uint32_t now = millis();
  webServer.handleClient();

  if (now - lastSimUpdate >= (uint32_t)SIM_UPDATE_MS) {
    lastSimUpdate = now;
    sim_moisture += (random(-5, 6) / 10.0f);
    if (sim_moisture < 20) sim_moisture = 20;
    if (sim_moisture > 80) sim_moisture = 80;
    sim_temp += (random(-5, 6) / 10.0f);
    if (sim_temp < 18) sim_temp = 18;
    if (sim_temp > 28) sim_temp = 28;
    sim_temp2 = sim_temp + (random(-3, 4) / 10.0f);
    sim_temp3 = sim_temp + (random(-4, 5) / 10.0f);
    if (sim_temp3 < 18) sim_temp3 = 18;
    if (sim_temp3 > 28) sim_temp3 = 28;
    sim_soil_temp += (random(-4, 5) / 10.0f);
    if (sim_soil_temp < 15) sim_soil_temp = 15;
    if (sim_soil_temp > 28) sim_soil_temp = 28;
    sim_soil_humidity += (random(-4, 5) / 10.0f);
    if (sim_soil_humidity < 25) sim_soil_humidity = 25;
    if (sim_soil_humidity > 85) sim_soil_humidity = 85;
    if (currentScreen == SCREEN_GRID)
      updateGridValues();
  }

  if (now - lastDraw >= 1000) {
    lastDraw = now;
    updateHeaderTime();
  }

  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    if (p.z < 300) {
      pendingHeaderBtn = -1;
      delay(10);
    } else {
      int tx = map(p.x, 200, 3700, 0, TFT_W);
      int ty = map(p.y, 200, 3800, 0, TFT_H);
      tx = constrain(tx, 0, TFT_W - 1);
      ty = constrain(ty, 0, TFT_H - 1);
#if TOUCH_DEBUG
      Serial.printf("touch raw x=%d y=%d z=%d -> tx=%d ty=%d cell=%d\n", p.x, p.y, p.z, tx, ty, hitCell(tx, ty));
#endif
      int btn = hitHeaderButton(tx, ty);
      if (btn >= 0) {
        if (pendingHeaderBtn == -1) {
          pendingHeaderBtn = btn;
          pendingHeaderStart = now;
        } else if (pendingHeaderBtn == btn && (now - pendingHeaderStart) >= (uint32_t)LONG_PRESS_MS) {
          if (btn == 0) {
            relay_fan = !relay_fan;
            digitalWrite(RELAY_FAN_PIN, relay_fan ? RELAY_ON : !RELAY_ON);
          } else {
            relay_water = !relay_water;
            digitalWrite(RELAY_WATER_PIN, relay_water ? RELAY_ON : !RELAY_ON);
          }
          drawHeader();
          pendingHeaderBtn = -1;
          delay(200);
        } else if (pendingHeaderBtn != btn) {
          pendingHeaderBtn = btn;
          pendingHeaderStart = now;
        }
      } else {
        pendingHeaderBtn = -1;
        if (currentScreen == SCREEN_GRID) {
          int cell = hitCell(tx, ty);
          if (cell >= 0) {
            screenGraph(cell);
          }
        } else if (currentScreen == SCREEN_GRAPH) {
          screenGrid();
        }
      }
      delay(50);
    }
  } else {
    pendingHeaderBtn = -1;
  }

  delay(50);
}
