# 🍡 Mochi DFPlayer

[![Build firmware](https://github.com/rz7mong/mochi-dfplayer/actions/workflows/firmware.yml/badge.svg)](https://github.com/rz7mong/mochi-dfplayer/actions/workflows/firmware.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Teman meja **ESP32-C3 Super Mini** + layar **ST7789 1,3" 240×240** dengan **suara MP3 lewat DFPlayer Mini**. Animasi disimpan sebagai **frame JPEG penuh warna di firmware**, jadi lebih tajam daripada GIF. Ada **pemutar MP3** di menu layar dan **Chronos (BLE)** untuk jam, notifikasi, panggilan, dan navigasi dari HP. Edisi bahasa Indonesia. Proyek independen yang terinspirasi Dasai Mochi, bukan produk resmi.

<p align="center">
  <img src="firmware/assets/builtin/gif/wajah/vid_00.gif" width="120" alt="wajah vid_00">
  <img src="firmware/assets/builtin/gif/wajah/vid_20.gif" width="120" alt="wajah vid_20">
  <img src="firmware/assets/builtin/gif/mobil/turbo.gif" width="120" alt="mobil turbo">
  <img src="firmware/assets/builtin/gif/mobil/headlights.gif" width="120" alt="mobil headlights">
</p>
<p align="center"><sub>GIF sumber tema wajah dan mobil. Di perangkat semuanya jadi frame JPEG.</sub></p>

**Firmware 0.7.0** · **MIT © rzmong** · Situs + pemasang browser: **https://rz7mong.github.io/mochi-dfplayer/** (sumber di [`docs/`](docs/))

## ✨ Fitur

- **30 model animasi** di flash: 10 wajah, 8 gundam, 4 mobil, 5 mochi, 3 dasai. JPEG lebar 240, tempo per klip. Saat nyala langsung `mochi/full1`.
- **Suara MP3 per model** dari kartu microSD DFPlayer (`/MP3/0001.mp3` …), **diputar sampai habis** lalu diulang. Tidak lagi dipotong tiap putaran gambar.
- **Pemutar MP3** di menu: putar/jeda, berikut/sebelum, volume, mode *ulang semua / ulang 1 / acak*, layar nomor trek dan volume. Lagu dari folder `/01`, terpisah dari suara animasi. Musik tetap jalan saat kembali ke animasi.
- **Chronos (BLE)**: jam dan tanggal dari HP, baterai HP, notifikasi, panggilan masuk, navigasi, cari perangkat.
- **Satu tombol sentuh**: ketuk, ketuk 2×, tahan, tahan 2 detik (menu).
- **Goyang** (opsional, MPU6050): klip "pusing" di tema yang sama.
- **Pengaturan tersimpan** (volume, rotasi, Chronos, Jam HP, mode musik, lagu terakhir).
- **Pasang dari browser** (ESP Web Tools) atau build sendiri dengan PlatformIO.

## ⚖️ Mochi DFPlayer vs Mochi rzmong

| | **Mochi DFPlayer** (repo ini) | [Mochi rzmong](https://github.com/rz7mong/mochi-rzmong) |
|---|---|---|
| Suara | MP3 lewat DFPlayer Mini | WAV lewat amplifier MAX98357 (I2S) |
| Animasi | **Lebih tajam**: frame JPEG penuh warna, ditanam di firmware | GIF dari kartu SD atau flash, kurang tajam |
| Tambah / ganti animasi | Build lalu flash ulang ([caranya](#tambah-animasi)) | **Salin GIF ke kartu SD**, tanpa flash ulang |
| Pemutar musik | **Ada** (menu LCD, folder `/01`) | Tidak ada |
| Chronos (BLE) | Ada, diatur dari menu LCD | Ada, diatur dari halaman web |
| Wi-Fi + halaman pengaturan | Tidak ada | Ada (`http://192.168.4.1/`) |
| Goyang (MPU6050) | Ada, opsional | Tidak ada |

> ⚠️ Firmware dan installer kedua repo **tidak bisa ditukar**. Installer mochi-rzmong untuk rakitan MAX98357, bukan DFPlayer.

## 🧺 Bahan

| Jumlah | Part | Catatan |
|---|---|---|
| 1 | ESP32-C3 Super Mini | flash 4 MB, USB-C, BLE |
| 1 | Layar ST7789 1,3" IPS 240×240 | SPI 7 pin (tanpa CS) atau 8 pin |
| 1 | DFPlayer Mini | slot microSD + amplifier 3 W |
| 1 | Speaker 8 Ω | 0,5–3 W |
| 1 | Kartu microSD | FAT32, ≤ 32 GB |
| 1 | Resistor 1 kΩ | jalur GPIO20 → RX DFPlayer |
| 1 | Tombol atau TTP223 | ke GPIO1, active-low ([catatan](#catatan-sentuh)) |
| 1 | MPU6050 | opsional, untuk goyang |

## 🔌 Kabel

Sumber: [`firmware/include/MochiRzmong.h`](firmware/include/MochiRzmong.h) dan [`User_Setup_ST7789.h`](firmware/include/User_Setup_ST7789.h).

| Kaki modul | ESP32-C3 | Catatan |
|---|---|---|
| TFT SCL / SCLK | GPIO4 | |
| TFT SDA / MOSI | GPIO6 | |
| TFT DC | GPIO3 | |
| TFT RES / RST | GPIO10 | |
| TFT CS | tidak dipakai | modul 8 pin: kaki CS ke GND |
| TFT BLK (lampu latar) | GPIO7 | HIGH nyala, LOW mati saat berhenti |
| TFT VCC | 3V3 | |
| Sentuh / tombol | GPIO1 | active-low (pull-up internal) |
| DFPlayer RX | GPIO20 lewat ±1 kΩ | ESP TX → RX modul |
| DFPlayer TX | GPIO21 | TX modul → ESP RX. **Wajib** agar "trek selesai" terdeteksi |
| DFPlayer BUSY | GPIO5 (opsional) | aktifkan dengan `-DMOCHI_PIN_DF_BUSY=5` |
| DFPlayer VCC | 5 V | dari 5V/VBUS, bukan 3V3 |
| DFPlayer SPK_1 / SPK_2 | speaker 8 Ω | tanpa amplifier tambahan |
| MPU6050 SDA / SCL | GPIO8 / GPIO9 | opsional, alamat 0x68, pull-up 4,7 kΩ ke 3V3 jika perlu |
| MPU6050 VCC | 3V3 | |
| GND | semua modul | |

- **GPIO9** pin strap: jangan ditarik ke GND saat boot.
- **GPIO20/21** adalah UART0 bawaan chip. Log ESP keluar lewat **USB CDC**, bukan UART0.

<a id="catatan-sentuh"></a>**Catatan sentuh:** firmware membaca GPIO1 sebagai **LOW = ditekan**. Modul TTP223 bawaan pabrik justru HIGH saat disentuh (seperti di mochi-rzmong). Solder jumper **A** di TTP223 agar active-low, atau build dengan `-DMOCHI_TOUCH_ACTIVE_HIGH`.

<a id="kartu-sd"></a>
## 💾 Kartu SD DFPlayer

Format **FAT32**, lalu buat struktur ini:

```
/MP3/0001.mp3 … 0040.mp3   suara animasi (nomor = trek model, lihat tabel)
/01/001.mp3 … 255.mp3      lagu untuk Pemutar MP3
/02/001.mp3                suara notifikasi Chronos
/03/001.mp3 …              nada dering panggilan Chronos (diulang)
```

- File dicocokkan dari **nama**, jadi urutan salin tidak penting. Nama folder `MP3`, `01`, `02`, `03`; nama file diawali nomor 4 digit (`/MP3`) atau 3 digit (folder angka).
- Nomor lagu di `/01` **harus berurutan tanpa lubang** (001, 002, 003 …). "Berikutnya" berhenti di lubang pertama lalu kembali ke 001.
- Di macOS, hapus file `._*` dan `.DS_Store`.
- Cek kartu: `python firmware/tools/daftar_trek.py --sd /path/ke/kartu`.

| Model (ketuk 2×) | File `/MP3/` | Klip |
|---|---|---|
| 1–10 | `0001`–`0010` | wajah vid_00, vid_01, vid_10, vid_11, vid_20, vid_21, vid_30, vid_31, vid_40, vid_41 |
| 11–14 | `0011`–`0014` | gundam intro, kokpit, tembak, siaga (tahan) |
| 15–18 | `0024`–`0027` | gundam hujan, isyarat, marah, ledak (goyang) |
| 19–22 | `0015`–`0018` | mobil car, turbo, headlights, speed_3 |
| 23–27 | `0019`–`0023` | mochi full1, chongmat1 (goyang), video17, video18, xoadau1 (tahan) |
| 28–30 | `0034`, `0038`, `0040` | dasai video03, video07, video2 (tahan) |

**Mode cadangan urutan salin** (`-DMOCHI_DF_COPY_ORDER`): suara animasi ditaruh di **root** (`0001.mp3` …) dan diputar menurut **urutan salin** FAT, bukan nama. Format kartu, salin file root berurutan **sebelum** folder lain. Tabel trek: `python firmware/tools/daftar_trek.py --copy-order`.

## 👆 Cara main

**Animasi** (layar utama)

| Gerakan | Hasil |
|---|---|
| Ketuk 1× | Putar / berhenti. Berhenti = layar hitam + lampu mati. Musik dari pemutar tetap jalan |
| Ketuk 2× (dalam 0,35 dtk) | Model berikutnya |
| Tahan ≥ 0,4 dtk | Klip "tahan" tema itu berulang sampai dilepas |
| Tahan 2 dtk | **Buka menu** |
| Goyang 3× dalam 1 dtk | Klip "goyang" tema itu sekali, lalu kembali (perlu MPU6050) |

Suara model diputar sampai habis lalu diulang. Klip goyang/tahan memutar suaranya sendiri; setelah selesai suara model utama mulai lagi dari awal. Saat pemutar musik aktif (main atau jeda), animasi tanpa suara.

**Menu** — ketuk 1× = pindah, ketuk 2× = pilih, tahan 2 dtk = tutup

| Item | Fungsi |
|---|---|
| Pemutar MP3 | Buka layar pemutar |
| Kembali ke animasi | Tutup menu (dan matikan Jam HP) |
| Volume + / Volume − | Langkah 2, rentang 0–30 (satu volume untuk animasi dan musik) |
| Chronos BLE | Nyala/mati Bluetooth untuk aplikasi Chronos |
| Jam HP | Layar jam dari HP sebagai layar utama (menyalakan Chronos) |
| Tampil navigasi | Tampilkan layar navigasi Chronos |
| Putar layar | Rotasi 0° / 90° / 180° / 270° |
| Tentang | Versi, nama BLE, status Chronos, baterai HP |

**Pemutar MP3** — layar menampilkan nomor trek, jumlah lagu, status, mode, volume, dan 7 tombol.

| Gerakan | Hasil |
|---|---|
| Ketuk 1× | Jalankan tombol yang disorot (langsung, tanpa jeda) |
| Tahan 0,4–2 dtk lalu lepas | Sorot tombol berikutnya |
| Tahan 2 dtk | Kembali ke menu (musik tetap jalan) |

Tombol berurutan: **⏯ putar/jeda · ⏭ berikutnya · ⏮ sebelumnya · 🔊+ · 🔉− · mode (ulang semua → ulang 1 → acak) · ■ berhenti**. Saat masuk, sorotan di ⏯. Ketuk berulang di 🔊+ untuk menaikkan volume cepat.

<a id="chronos"></a>
## 📱 Chronos

1. Pasang aplikasi **Chronos** (fbiego) di HP Android.
2. Di perangkat: tahan 2 dtk → menu → **Chronos BLE** → ketuk 2× (jadi ON).
3. Di aplikasi Chronos, sambungkan perangkat **`rzmong dfplayer`**.
4. Opsional: menu → **Jam HP** untuk layar jam (tanggal, hari, jam:menit, baterai HP).

| Dari HP | Di perangkat | Sentuh |
|---|---|---|
| Notifikasi | Layar notifikasi 6 dtk + suara `/02/001.mp3` | ketuk = tutup |
| Panggilan masuk | Layar panggilan + dering `/03` | tahan = tutup (dering berhenti) |
| Navigasi yang diteruskan aplikasi Chronos | Ikon arah, jarak, petunjuk, ETA | ketuk 2× = sembunyikan |
| Cari perangkat | Layar berkedip + suara notifikasi | |
| Waktu, baterai HP | Layar Jam HP dan Tentang | |

Chronos mati secara bawaan (hemat daya). Notifikasi dan panggilan memotong musik; setelah selesai, lagu diulang dari awal trek.

## ⚡ Flash

**Dari browser:** buka **https://rz7mong.github.io/mochi-dfplayer/** di Chrome/Edge, tahan BOOT, colok USB-C, klik **Install** (centang *Erase* saat naik dari versi < 0.7.0). Isinya [`docs/firmware/firmware.bin`](docs/firmware/) (gabungan bootloader + partisi + aplikasi, offset 0x0) dan [`manifest.json`](docs/firmware/manifest.json).

**Build sendiri (PlatformIO):**

```bash
pip install platformio pillow
cd firmware
pio run -e esp32-c3-dfplayer -t upload
pio device monitor        # 115200, lewat USB CDC
```

Saat build, `extra_script.py` menjalankan `tools/embed_jpeg.py` yang menanam klip ke `include/jpeg_clips.h`.

**Opsi build** (tambahkan ke `build_flags` di `platformio.ini`):

| Flag | Fungsi |
|---|---|
| `-DMOCHI_DF_COPY_ORDER` | Suara animasi di root, menurut urutan salin (mode lama) |
| `-DMOCHI_PIN_DF_BUSY=5` | Pakai pin BUSY DFPlayer di GPIO5 untuk deteksi trek selesai |
| `-DMOCHI_TOUCH_ACTIVE_HIGH` | Sentuh HIGH = ditekan (TTP223 bawaan pabrik), dengan pull-down |
| `-DMOCHI_DEFAULT_ROTATION=2` | Rotasi awal layar (0–3). Menu "Putar layar" menimpanya |
| `-DMOCHI_BLE_NAME=\"nama\"` | Nama perangkat di aplikasi Chronos |

**Ukuran flash.** App 0x3F0000 (4.128.768 B, partisi terbesar di flash 4 MB). Build 0.7.0: **94,9%** flash, RAM 12,4%. Klip JPEG mentah 3,71 MB tidak muat bersama BLE, jadi `embed_jpeg.py` punya **anggaran** (`custom_jpeg_budget = 3300000` di `platformio.ini`). Jika total klip melebihinya, frame yang **nyaris sama** dengan frame sebelumnya (saat ini ≤ 1,1% piksel berbeda) dipakai ulang. Jumlah frame dan tempo tetap; file aset tidak diubah. Hasilnya tercetak saat build (`ANGGARAN: …`).

**Perbarui `docs/firmware/firmware.bin` setelah build:**

```bash
B=firmware/.pio/build/esp32-c3-dfplayer
python -m esptool --chip esp32c3 merge_bin -o docs/firmware/firmware.bin --flash_mode dio --flash_size 4MB \
  0x0 $B/bootloader.bin 0x8000 $B/partitions.bin \
  0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 $B/firmware.bin
```

Workflow [`firmware.yml`](.github/workflows/firmware.yml) membuat file yang sama sebagai artefak di setiap push/PR (tanpa commit otomatis).

<a id="tambah-animasi"></a>
## 🎞️ Tambah animasi baru

Animasi ditanam di firmware, jadi alurnya **impor → daftarkan → build → flash ulang**:

1. Impor klip (GIF, folder JPEG/PNG, atau header C) menjadi `firmware/assets/builtin/jpeg/<tema>/<nama>.mjpeg` + `.json`:
   ```bash
   cd firmware
   python tools/import_jpeg_clip.py <tema> <nama> sumber.gif --quality 80
   python tools/import_jpeg_clip.py <tema> <nama> folder_frame/
   ```
   Opsi: `--resize`, `--crop`, `--step`, `--hold`, `--delay` (lihat `--help`). Frame lebih kecil dari 240×240 digambar di tengah.
2. Daftarkan di `firmware/assets/meta.json` → `"jpeg_clips"`: `["<tema>", "<nama>", "", <trek>]`. Peran: `""` biasa, `"dizzy"` saat goyang, `"heart"` saat tahan. Trek boleh dikosongkan (otomatis nomor berikutnya).
3. Build + flash: `pio run -e esp32-c3-dfplayer -t upload`. Perhatikan baris `ANGGARAN` jika klip banyak.
4. `python tools/daftar_trek.py` untuk melihat nomor trek, lalu salin `/MP3/000N.mp3` ke kartu.

Aturan: klip satu tema selalu dikelompokkan; tema baru masuk di akhir urutan model. `meta.json` `"boot_theme"` menentukan model saat nyala. Pakai hanya media yang boleh kamu sebarkan ([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)).

## 🛠️ Kalau ada masalah

| Gejala | Cek |
|---|---|
| Port tidak muncul | Chrome/Edge di komputer, kabel USB data, tahan BOOT saat colok |
| Layar hitam | SCLK 4, MOSI 6, DC 3, RST 10, BLK 7; CS modul 8 pin ke GND. Mode berhenti juga hitam: ketuk 1× |
| Gambar terbalik/miring | Menu → Putar layar, atau `-DMOCHI_DEFAULT_ROTATION` |
| Tidak ada suara | DFPlayer VCC 5 V, GPIO20 →(1 kΩ)→ RX, TX → GPIO21, speaker SPK_1/SPK_2, kartu FAT32, folder `/MP3`. Serial: `DFPlayer menjawab` |
| Suara tidak diulang / lagu tidak lanjut | Kabel TX modul → GPIO21 (pesan "trek selesai"), atau pasang BUSY + `-DMOCHI_PIN_DF_BUSY=5` |
| Pemutar: "folder /01 kosong" | Lagu harus `/01/001.mp3`, `/01/002.mp3`, … |
| Ketukan tidak terbaca / selalu "tahan" | Polaritas sentuh, lihat [catatan sentuh](#catatan-sentuh) |
| Chronos tidak menemukan perangkat | Menu → Chronos BLE harus ON. Nama `rzmong dfplayer` |
| Goyang tidak bereaksi | Serial harus menulis `MPU6050 siap`. SDA 8, SCL 9 |
| Build: `tidak muat di anggaran` | Klip terlalu banyak. Kurangi klip atau atur `custom_jpeg_budget` (lalu cek ukuran app) |

## 📁 Isi repo

| Path | Isi |
|---|---|
| `firmware/src/main.cpp` | Animasi, sentuh, goyang, menu, pemutar MP3, jam, overlay Chronos |
| `firmware/include/chronos_ui.inc` | Layar + callback Chronos (dari mochi-rzmong) |
| `firmware/lib/MochiDfPlayer/` | DFPlayer: trek animasi, folder musik, notifikasi, dering, deteksi trek selesai |
| `firmware/assets/` | Klip JPEG (`builtin/jpeg/`), GIF sumber, `meta.json` |
| `firmware/tools/` | `import_jpeg_clip.py`, `embed_jpeg.py`, `daftar_trek.py` |
| `docs/` | Situs GitHub Pages + pemasang browser (`docs/firmware/`) |

MIT © rzmong untuk kode. Klip gundam, dasai, dan mochi punya catatan hak tersendiri di [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
