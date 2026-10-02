# Mochi DFPlayer

Edisi bahasa Indonesia untuk ESP32-C3 Super Mini + layar ST7789 1,3 inci 240×240, suara lewat DFPlayer Mini. Cara main mengikuti pola Dasai Mochi, tetapi teks dan kabel milik repo ini. Tema `mochi` berisi 5 klip JPEG yang ditambahkan pemilik repo (lihat THIRD_PARTY_NOTICES).

**MIT © rzmong**

Rakitan: [docs/index.html](docs/index.html). Instalasi: [docs/instalasi.html](docs/instalasi.html).

## Build

```bash
cd firmware
pio run -e esp32-c3-dfplayer -t upload
```

Tahan BOOT saat colok USB-C jika unduhan gagal. Port serial memakai USB CDC.

## Kabel

| Net | ESP32-C3 |
|---|---|
| TFT SCLK | GPIO4 |
| TFT MOSI | GPIO6 |
| TFT DC | GPIO3 |
| TFT RST | GPIO10 |
| TFT CS | tidak disambung, kaki CS modul ke GND |
| Lampu latar | GPIO7, HIGH nyala, LOW mati |
| Sentuh | GPIO1, active-low |
| MPU6050 SDA / SCL | GPIO8 / GPIO9 |
| DFPlayer RX | GPIO20 lewat resistor sekitar 1 kΩ |
| DFPlayer TX | GPIO21 |
| DFPlayer VCC | 5 V |
| Speaker | SPK+ / SPK−, 8 Ω |

GPIO9 pin strap. Jangan tarik ke GND saat boot.

## Cara main

- Ketuk singkat: putar atau berhenti. Berhenti menghitamkan layar dan mematikan lampu.
- Ketuk dua kali: model berikutnya.
- Tahan: klip terakhir tema itu berulang sampai dilepas. Di tema `mochi`: xoadau1 (mata hati).
- Goyang tiga kali dalam 1 detik: klip lain di tema yang sama, sekali, lalu kembali. Di tema `mochi`: chongmat1 (mata pusing).

Dua puluh tiga model di flash: 10 wajah, 4 gundam, 4 mobil, 5 mochi. Saat nyala langsung memutar `mochi/full1`. Kartu DFPlayer FAT32, berkas di root: `0001.mp3` sampai `0023.mp3`. Nomor sama dengan urutan model. Volume 28.

### Tema mochi

| Model | Trek | Klip | Frame |
|---|---|---|---|
| 19 | `0019.mp3` | full1, wajah utama | 558 |
| 20 | `0020.mp3` | chongmat1, mata pusing (goyang) | 44 |
| 21 | `0021.mp3` | video17, senyum + lampu sorot | 132 |
| 22 | `0022.mp3` | video18, mata sirene merah/biru | 72 |
| 23 | `0023.mp3` | xoadau1, mata hati (tahan) | 19 |

Semua frame tetap ada, 100 ms per frame. Sumber di `firmware/assets/builtin/jpeg/mochi/` (`.mjpeg` + `.json`), dibuat dengan `tools/import_jpeg_clip.py`: frame yang nyaris sama dengan frame sebelumnya dipakai ulang, lalu `jpegtran -optimize` lossless. `tools/embed_jpeg.py` menanamnya ke flash saat build.

Tidak ada Wi-Fi, menu, atau Chronos di firmware ini.
