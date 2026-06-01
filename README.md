# GRENSAN – demo UI (lokalno)

ESP32 prikaz za rastlinjak: **dotik, simulirani senzorji, brez WiFi/MQTT/strežnikov**. Vse teče lokalno na ploščici; primeren za razvoj UI in kasnejšo integracijo s pravo infrastrukturo.

## Kaj vključuje

- **Header:** ura (od zagona), naslov GRENSAN, gumba FAN / IRR (dolg pritisk 0,5 s)
- **Mreža 6 senzorjev (simulacija):** Vlaga, Temp1–3, Soil temp, Soil hum
- **Graf:** klik na celico; klik kamorkoli na grafu → nazaj
- **Releji:** GPIO 27 (ventilator), GPIO 26 (zalivanje)

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
pio run -t upload
```

Po prvem buildu, če zaslon ni pravilen, prepiši `User_Setup.h`:

```powershell
copy firmware\User_Setup.h firmware\.pio\libdeps\esp32dev\TFT_eSPI\User_Setup.h
pio run -t upload
```

V `firmware/platformio.ini` nastavi `upload_port` na svoj COM port.

## Konfiguracija

Pini in interval simulacije: [`firmware/include/config.h`](firmware/include/config.h).

## Opomba o dveh okoljih

Ta repozitorij je namenjen **lokalnemu demo UI**. Infrastruktura (WiFi, MQTT, Modbus, Home Assistant) naj ostane v ločenem projektu/veji, ko jo boš dodajal – tukaj je namenoma izpuščena, da je koda varna za Git in enostavna za razvoj brez strežnikov.
