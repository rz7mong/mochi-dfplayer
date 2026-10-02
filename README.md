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
- Tahan: klip terakhir tema itu berulang sampai dilepas. Di tema `mochi`: xoadau1 (mata hati). Gundam: siaga. Dasai: video2.
- Goyang tiga kali dalam 1 detik: klip lain di tema yang sama, sekali, lalu kembali. Di tema `mochi`: chongmat1 (mata pusing). Gundam: ledak.

Tiga puluh model di flash, urutan ketuk dua kali: 10 wajah, 8 gundam, 4 mobil, 5 mochi, 3 dasai. Semua klip JPEG penuh, lebar 240. Saat nyala langsung memutar `mochi/full1`. Kartu DFPlayer FAT32, berkas di root. Nomor trek tetap per klip (bukan urutan model). Volume 28.

| Model | Trek | Klip |
|---|---|---|
| 1–10 | `0001`–`0010` | wajah vid_00 … vid_41 |
| 11–14 | `0011`–`0014` | gundam intro, kokpit, tembak, siaga (tahan) |
| 15–18 | `0024`–`0027` | gundam hujan, isyarat (gestur jari tengah), marah, ledak (goyang) |
| 19–22 | `0015`–`0018` | mobil car, turbo, headlights, speed_3 |
| 23–27 | `0019`–`0023` | mochi full1, chongmat1, video17, video18, xoadau1 |
| 28–30 | `0034`, `0038`, `0040` | dasai video03 (lampu sorot), video07 (spidometer), video2 (tahan) |

Trek 28–31 cadangan gundam; 32–33, 35–37, 39, 41–46 tidak dipakai lagi.

### Tema wajah dan mobil

Semua frame GIF sumber (5–8 frame, sebelumnya dipotong jadi 6), 240×240, JPEG kualitas 80. Frame yang sama dipakai ulang.

### Tema mochi

full1 (558 frame) wajah utama, chongmat1 (44) saat goyang, video17 (132), video18 (72), xoadau1 (19) saat tahan. 240×240, semua frame, tanpa encode ulang (hanya optimasi lossless), 100 ms per frame.

### Tema gundam

Dasai Mochi edisi Gundam dari `gundam.mp4` di [huykhoong/esp32_dasai_mochi_clone_and_how_to](https://github.com/huykhoong/esp32_dasai_mochi_clone_and_how_to) (320×240, 15 fps, 246 frame), dipotong per adegan menjadi 8 klip: intro (frame 0–13), kokpit (14–69), tembak (70–105), siaga (106–117), hujan (118–134), isyarat (135–165), marah (166–197), ledak (198–245). Potong tengah 228×228 dari 320×240, Lanczos ke 240×240, unsharp ringan, JPEG kualitas 72. Agar muat diambil 1 dari 3 frame (5 fps, durasi sama). Goyang: ledak. Tahan: siaga. Catatan: adegan isyarat berisi gestur jari tengah.

### Tema dasai

Hanya 3 klip dari bangdc90/dasai_mochi_tft: dua yang berunsur mobil (video03 lampu sorot, video07 spidometer) dan video2 (ekspresi sentuh saat ditahan). Sumber 160×80 diperbesar Lanczos ke 240×120 (di tengah vertikal) dengan unsharp ringan, JPEG kualitas 75, 1 dari 3 frame (120 ms).

### Cara menambah klip

Sumber ada di `firmware/assets/builtin/jpeg/<tema>/` (`.mjpeg` + `.json`), dibuat `tools/import_jpeg_clip.py` (header C, folder JPEG, atau GIF). Frame yang nyaris sama dipakai ulang, JPEG dioptimasi `jpegtran`. Daftarkan di `meta.json` `jpeg_clips`. `tools/embed_jpeg.py` menanamnya saat build.

Partisi: satu app `0x3F0000` (4.032 KB), tanpa spiffs.

Tidak ada Wi-Fi, menu, atau Chronos di firmware ini.
