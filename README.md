# Mochi DFPlayer

Varian **ESP32-C3 Super Mini + ST7789 1.3" 240×240** yang suaranya lewat **DFPlayer Mini** (MP3). Firmware layar, tema, sentuh, dan Chronos mengikuti [mochi-rzmong](https://github.com/rz7mong/mochi-rzmong). Build WAV + MAX98357 tetap di repo itu, tidak dicampur di sini.

**MIT © rzmong**

## Build

```bash
cd firmware
pio run -e esp32-c3-dfplayer -t upload
```

## Kabel

Lepas MAX98357. GPIO20/21 dipakai UART, bukan I2S.

| DFPlayer | ESP32-C3 |
|---|---|
| VCC | 5 V |
| GND | GND |
| RX | GPIO20 lewat resistor ~1 kΩ |
| TX | GPIO21 |
| SPK+ / SPK− | speaker 8 Ω |

TFT, SD GIF, dan sentuh sama dengan mochi-rzmong.

## SD modul (FAT32)

| Folder | Isi |
|---|---|
| `01/001.mp3` .. `011.mp3` | reaksi: raspberry, squint, love_hearts_kiss, angry_2, smirk, sleepy, yawn_tired, rainbow, pong, revs, hadouken_hit |
| `02/001.mp3` .. | ekspresi wajah, nomor = indeks + 1 |
| `06/001.mp3` .. `009.mp3` | tema: wajah, gundam, mobil, polisi, musik, neon, anime, makanan, intro |
| `03/001.mp3` | notifikasi Chronos |
| `04/001.mp3` | dering Chronos, diulang sampai ditutup |
| `05/001.mp3` .. | lagu pemutar |

Menu: Musik putar/jeda, lagu berikut, lagu sebelumnya. Dari AP: `POST http://192.168.4.1/api/music` dengan `{"action":"play"}`, `next`, `prev`, `toggle`, atau `stop`.

Library: `firmware/lib/MochiDfPlayer`.
