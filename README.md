# Mochi DFPlayer

Edisi bahasa Indonesia untuk ESP32-C3 Super Mini + layar ST7789 1,3 inci 240×240, suara lewat DFPlayer Mini. Cara main mengikuti pola Dasai Mochi, tetapi teks dan kabel milik repo ini. Tema `wajah`, `mobil`, dan `gundam` berisi klip JPEG yang ditambahkan pemilik repo (lihat THIRD_PARTY_NOTICES).

**MIT © rzmong**

Panduan lengkap (komponen, kabel, upload, kartu SD, cara pakai, masalah, menambah klip): [docs/instalasi.html](docs/instalasi.html).

## Ringkas

**Komponen:** ESP32-C3 Super Mini, LCD ST7789 1,3 inci 240×240 SPI, DFPlayer Mini + microSD FAT32, speaker 8 Ω, MPU6050 (GY-521), sensor sentuh TTP223 aktif LOW (pad A dijembatani) atau tombol ke GND, resistor 1 kΩ.

| Modul | Pin modul → ESP32-C3 |
|---|---|
| LCD | VCC 3V3, SCL GPIO4, SDA GPIO6, RES GPIO10, DC GPIO3, BLK GPIO7, CS (jika ada) ke GND |
| DFPlayer | VCC 5V, RX ← GPIO20 lewat 1 kΩ, TX → GPIO21, SPK_1/SPK_2 ke speaker |
| MPU6050 | VCC 3V3, SDA GPIO8, SCL GPIO9 |
| Sentuh | I/O GPIO1 (LOW = disentuh) |

Semua GND disatukan. GPIO8/GPIO9 pin strap, jangan ditarik ke GND saat boot.

**Upload** (perlu Python 3 + Pillow untuk skrip build):

```bash
pip install platformio pillow
git clone https://github.com/rz7mong/mochi-dfplayer.git
cd mochi-dfplayer/firmware
pio run -e esp32-c3-dfplayer -t erase    # sekali, partisi berubah
pio run -e esp32-c3-dfplayer -t upload
```

Tahan BOOT saat colok USB-C jika upload gagal. Belum ada pemasang lewat browser.

**Kartu SD:** FAT32, berkas di root. Yang berbunyi: `0019`–`0023`, `0026`–`0031`, `0034`, `0038`, `0040` (.mp3, 14 berkas). DFPlayer memilih trek dari urutan salin, bukan nama, jadi salin `0001.mp3`–`0040.mp3` berurutan ke kartu baru diformat; nomor yang tidak dipakai diisi MP3 hening. Detail di panduan.

## Cara main

- Ketuk singkat: putar atau berhenti. Berhenti menghitamkan layar dan mematikan lampu.
- Ketuk dua kali: model berikutnya.
- Tahan (≥ 0,4 detik): klip tahan tema itu berulang sampai dilepas. Wajah: cinta (mata hati). Mobil: speedometer. Gundam: helm_siaga.
- Goyang tiga kali dalam 1 detik: klip goyang diputar sekali, lalu kembali. Wajah: pusing. Mobil dan gundam: klip berikutnya di tema itu (di speedometer, speedometer diulang).

Empat belas model di flash, urutan ketuk dua kali: 6 wajah, 2 mobil, 6 gundam. Saat nyala langsung memutar `wajah/senyum_kedip` (model 1). Nomor trek tetap per klip (bukan urutan model). Volume 28 dari 30.

| Model | Trek | Klip |
|---|---|---|
| 1 | `0019` | wajah/senyum_kedip (awal) |
| 2 | `0020` | wajah/pusing (goyang) |
| 3 | `0023` | wajah/cinta (tahan) |
| 4 | `0021` | wajah/sorot |
| 5 | `0022` | wajah/sirine |
| 6 | `0040` | wajah/cinta_pipi |
| 7 | `0034` | mobil/lampu_sorot |
| 8 | `0038` | mobil/speedometer (tahan) |
| 9–14 | `0026`–`0031` | gundam/helm_hujan, helm_siaga (tahan), isyarat, kokpit, kokpit_2, pilot |

### Tema wajah

senyum_kedip (558 frame) wajah utama, pusing (44) saat goyang, cinta (19) saat tahan, sorot (132), sirine (72): berkas milik pemilik repo, 240×240, semua frame, tanpa encode ulang (hanya optimasi lossless), 100 ms per frame. cinta_pipi (38 frame) dari bangdc90/dasai_mochi_tft, 240×120 seperti tema mobil.

### Tema gundam

6 GIF penuh dari paket tema `assets-v1` rz7mong/mochi-rzmong (`mochi-themes.zip`, `gif/gundam/`): helm_hujan, helm_siaga, isyarat, kokpit, kokpit_2, pilot. Semua frame (82) dengan durasi frame GIF asli (90 ms). 240×240: dither GIF dihaluskan (blur Gaussian 1,2 px), Lanczos, unsharp ringan, JPEG kualitas 82. Tahan: helm_siaga. Model GIF gundam lama (blade, titan, hadouken_hit, mecha_doc) tidak ditanam (`skip_builtins` di `meta.json`).

### Tema mobil

lampu_sorot dan speedometer dari bangdc90/dasai_mochi_tft. Sumber 160×80 diperbesar Lanczos ke 240×120 (di tengah vertikal) dengan unsharp ringan, JPEG kualitas 80, semua frame (40 ms, 25 fps). lampu_sorot 167 frame, speedometer 106.

### Cara menambah klip

`tools/import_jpeg_clip.py` membuat `firmware/assets/builtin/jpeg/<tema>/<nama>.mjpeg` + `.json` dari header C, folder JPEG/PNG, atau GIF. Daftarkan di `meta.json` `jpeg_clips` sebagai `["tema", "nama", peran, trek]` (peran `""`, `"dizzy"`, atau `"heart"`). Urutan tema model diatur `theme_order`, tema saat nyala `boot_theme`. `tools/embed_jpeg.py` menanamnya saat build. Langkah dan contoh: [panduan bagian 7](docs/instalasi.html#klip).

Model GIF lama (wajah vid_*, mobil car/turbo/headlights/speed_3, gundam blade/titan/hadouken_hit/mecha_doc) tidak ditanam (`skip_builtins` di `meta.json`); GIF-nya tetap di `assets/builtin/gif/` karena dibaca `embed_assets.py`.

Partisi: satu app `0x3F0000` (4.032 KB), tanpa spiffs.

Tidak ada Wi-Fi, menu, atau Chronos di firmware ini.
