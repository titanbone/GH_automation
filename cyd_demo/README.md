# CYD demo – TFT_eSPI + XPT2046 touch

Enaka logika kot Arduino **"borders with touch.ino"**: rotation 1, invertDisplay(true), robovi, številke 1–5, dotik za risanje.

## Nastavitve (User_Setup.h)

V mapi projekta je **User_Setup.h** (kopija tvojega `User_Setup_OK.h`) z:

- **ILI9341_2_DRIVER** (cel zaslon 240×320)
- TFT: CS=15, DC=2, RST=-1, SCLK=14, MOSI=13, MISO=12, BL=21
- Touch CS=33, USE_HSPI_PORT za TFT

Če po prvem `pio run` zaslon še vedno nima prave orientacije ali deluje le del zaslona, **prepiši** User_Setup knjižnice z našim:

```powershell
copy cyd_demo\User_Setup.h cyd_demo\.pio\libdeps\esp32dev\TFT_eSPI\User_Setup.h
pio run -t upload
```

Nato naloži ponovno.

## Ukazi

```powershell
cd c:\Users\HP\Cursor\GH_automation\cyd_demo
pio run
pio run -t upload
pio device monitor -b 115200
```

## Pini (E32R28T / CYD)

- TFT: CS=15, DC=2, RST=-1, SCLK=14, MOSI=13, MISO=12, BL=21  
- Touch: CS=33, IRQ=36, CLK=25, MOSI=32, MISO=39
