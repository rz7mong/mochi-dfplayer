# Mochi DFPlayer

Varian **ESP32-C3 Super Mini + ST7789 1.3" 240×240** yang suaranya lewat **DFPlayer Mini** (MP3). Firmware layar, tema, sentuh, dan Chronos mengikuti [mochi-rzmong](https://github.com/rz7mong/mochi-rzmong). Build WAV + MAX98357 tetap di repo itu, tidak dicampur di sini.

**MIT © rzmong**

Halaman rakitan: [docs/index.html](docs/index.html). Instalasi: [docs/instalasi.html](docs/instalasi.html).

## Build

```bash
cd firmware
pio run -e esp32-c3-dfplayer -t upload
```

## Kabel

Pinout sama dengan Dasai Mochi pikapet / [bangdc90/esp32-c3-phatvideo_anime](https://github.com/bangdc90/esp32-c3-phatvideo_anime). Jangan pakai kabel mochi-rzmong (CS GPIO7, DC GPIO10, RST GPIO0).

| Net | ESP32-C3 |
|---|---|
| TFT SCLK | GPIO4 |
| TFT MOSI | GPIO6 |
| TFT DC | GPIO3 |
| TFT RST | GPIO10 |
| TFT CS | tidak disambung |
| Backlight | GPIO7 (HIGH = nyala) |
| Sentuh TTP223 | GPIO1 |
| MPU6050 SDA / SCL | GPIO8 / GPIO9 |
| DFPlayer RX | GPIO20 lewat resistor ~1 kΩ |
| DFPlayer TX | GPIO21 |
| DFPlayer VCC | 5 V |
| Speaker | SPK+ / SPK− modul, 8 Ω |

GPIO9 pin strap. Jangan ditarik ke GND saat boot.

Gambar memakai 18 klip JPEG di flash: 10 wajah, 4 gundam, 4 mobil. Web dan menu hanya menampilkan tema itu. Trek DFPlayer folder 02 nomor 1-18 sama dengan urutan klip.

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
