# Mochi DFPlayer

Edisi bahasa Indonesia untuk ESP32-C3 Super Mini + layar ST7789 1,3 inci 240×240, suara lewat DFPlayer Mini. Cara main mengikuti pola Dasai Mochi, tetapi teks dan kabel milik repo ini. Tema `mochi`, `gundam`, dan `dasai` berisi klip JPEG yang ditambahkan pemilik repo (lihat THIRD_PARTY_NOTICES).

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
- Tahan: klip terakhir tema itu berulang sampai dilepas. Di tema `mochi`: xoadau1 (mata hati). Di tema `gundam`: mecha_doc.
- Goyang tiga kali dalam 1 detik: klip lain di tema yang sama, sekali, lalu kembali. Di tema `mochi`: chongmat1 (mata pusing).

Empat puluh dua model di flash, urutan ketuk dua kali: 10 wajah, 12 gundam, 4 mobil, 5 mochi, 11 dasai. Saat nyala langsung memutar `mochi/full1`. Kartu DFPlayer FAT32, berkas di root `0001.mp3` sampai `0042.mp3`. Nomor trek tetap per klip (bukan urutan model), jadi trek lama tidak bergeser. Volume 28.

| Model | Trek | Klip |
|---|---|---|
| 1–10 | `0001`–`0010` | wajah vid_00 … vid_41 (GIF, 6 frame) |
| 11–14 | `0011`–`0014` | gundam blade, titan, hadouken_hit, mecha_doc (versi penuh, menggantikan versi 6 frame) |
| 15–22 | `0024`–`0031` | gundam equip, hadouken_miss, helm_hujan, helm_siaga, isyarat, kokpit, kokpit_2, pilot |
| 23–26 | `0015`–`0018` | mobil car, turbo, headlights, speed_3 (GIF, 6 frame) |
| 27–31 | `0019`–`0023` | mochi full1, chongmat1, video17, video18, xoadau1 |
| 32–42 | `0032`–`0042` | dasai video01–video08, video2, video11, video12 |

### Tema mochi

full1 (558 frame) wajah utama, chongmat1 (44) saat goyang, video17 (132), video18 (72), xoadau1 (19) saat tahan. Semua frame, 100 ms per frame.

### Tema gundam

12 klip penuh dari paket tema `assets-v1` rz7mong/mochi-rzmong (`mochi-themes.zip`, `gif/gundam/`). Durasi frame GIF asli dijaga (60–90 ms, frame panjang diulang). Dither GIF dihaluskan (blur 1,2 px) lalu JPEG kualitas 60. Tahan: mecha_doc.

### Tema dasai

Klip 160×80 dari bangdc90/dasai_mochi_tft, digambar di tengah layar dengan latar hitam (tidak diperbesar). Diambil 1 dari 2 frame (80 ms per frame, durasi sama), JPEG kualitas 70 (video11/12: 65). video09, video10, video13, video14 tidak muat di flash.

### Cara menambah klip

Sumber ada di `firmware/assets/builtin/jpeg/<tema>/` (`.mjpeg` + `.json`), dibuat `tools/import_jpeg_clip.py` (header C, folder JPEG, atau GIF). Frame yang nyaris sama dipakai ulang, JPEG dioptimasi `jpegtran`. Daftarkan di `meta.json` `jpeg_clips`. `tools/embed_jpeg.py` menanamnya saat build.

Partisi: satu app `0x3F0000` (4.032 KB), tanpa spiffs.

Tidak ada Wi-Fi, menu, atau Chronos di firmware ini.
