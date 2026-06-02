/**
 * Konfiguracija – lokalni demo UI.
 * ESP32 + TFT ILI9341 + touch XPT2046, releji FAN/IRR + AP web server.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ----- Releji (25 = touch CLK; ventilator 27, voda 26) -----
#define RELAY_FAN_PIN   27
#define RELAY_WATER_PIN 26
#define RELAY_ON        HIGH

// ----- Prikaz 240×320 (ILI9341) – E32R28T / CYD -----
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1
#define TFT_SCLK 14
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_BL   21

// Touch XPT2046
#define TOUCH_CS    33
#define TOUCH_IRQ   36
#define TOUCH_CLK   25
#define TOUCH_MOSI  32
#define TOUCH_MISO  39

// ----- UI -----
#define TOOLBAR_H  40

// Posodabljanje simuliranih vrednosti senzorjev
#define SIM_UPDATE_MS  2000

// ----- WiFi AP + Web UI -----
// Telefon/PC se poveže direktno na ta AP in odpre http://192.168.4.1
#define WEB_AP_SSID      "GRENSAN-DEMO"
#define WEB_AP_PASSWORD  "grensan123"
#define WEB_SERVER_PORT  80

#endif

