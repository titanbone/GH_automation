# GRENSAN – demo UI (lokalno + web)

ESP32 prikaz za rastlinjak: **dotik, simulirani senzorji in lokalni web dashboard**. Deluje brez zunanjih strežnikov (brez MQTT/HA), primeren za razvoj in testiranje UI.

## Kaj vključuje (trenutno)

- **Header:** ura (sinhronizirana z lokalnim časom telefona/PC ob odprtju web strani), naslov GRENSAN, gumba FAN / IRR (dolg pritisk 0,5 s)
- **Mreža 6 senzorjev (simulacija):** Vlaga, Temp1–3, Soil temp, Soil hum
- **Graf:** klik na celico; klik kamorkoli na grafu → nazaj
- **Releji:** GPIO 27 (ventilator), GPIO 26 (zalivanje)
- **Web dashboard (AP mode):**
  - SSID: `GRENSAN-DEMO`
  - Password: `grensan123`
  - URL: `http://192.168.4.1`
  - API status: `http://192.168.4.1/api/status`

## Struktura repozitorija

| Mapa | Opis |
|------|------|
| `firmware/` | Glavni projekt (PlatformIO) – uporabi ta za nalaganje |
| `cyd_demo/` | Opcijski test zaslona/toucha (okvirji, risanje na dotik) |

## Zahteve

- ESP32 z 2.8" ILI9341 + XPT2046 (npr. CYD / E32R28T)
- [PlatformIO](https://platformio.org/) (razširitev v Cursor/VSCode ali CLI)

## Build in upload

```powershell
cd firmware
pio run
pio run -t upload --upload-port COM3
```

Po prvem buildu, če zaslon ni pravilen, prepiši `User_Setup.h`:

```powershell
copy firmware\User_Setup.h firmware\.pio\libdeps\esp32dev\TFT_eSPI\User_Setup.h
pio run -t upload
```

V `firmware/platformio.ini` nastavi `upload_port` na svoj COM port (pri tebi je trenutno deloval `COM3`).

## Test na telefonu (Chrome)

1. Na telefonu se poveži na Wi‑Fi: `GRENSAN-DEMO`
2. V Chrome odpri: `http://192.168.4.1`
3. Po odprtju strani se ura na TFT samodejno uskladi z lokalnim časom telefona.

## Konfiguracija

Pini in interval simulacije: [`firmware/include/config.h`](firmware/include/config.h).

## Opomba o dveh okoljih

Ta repozitorij je namenjen **lokalnemu demo UI + lokalnemu AP web prikazu**. Infrastruktura (WiFi klient, MQTT, Modbus, Home Assistant) naj ostane v ločeni veji/projektu, da ostane ta verzija enostavna in varna za testiranje.
