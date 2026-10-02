# 🍡 Mochi DFPlayer

[![Build firmware](https://github.com/rz7mong/mochi-dfplayer/actions/workflows/firmware.yml/badge.svg)](https://github.com/rz7mong/mochi-dfplayer/actions/workflows/firmware.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Teman meja **ESP32-C3 Super Mini** + layar **ST7789 1,3" 240×240** dengan **suara MP3 lewat DFPlayer Mini**. Animasi disimpan sebagai **frame JPEG penuh warna di firmware**, jadi lebih tajam daripada GIF. Ada **pemutar MP3** di menu layar dan **Chronos (BLE)** untuk jam, notifikasi, panggilan, dan navigasi dari HP. Edisi bahasa Indonesia. Proyek independen yang terinspirasi Dasai Mochi, bukan produk resmi.

<p align="center">
  <img src="docs/img/senyum_kedip.gif" width="120" alt="wajah senyum_kedip">
  <img src="docs/img/cinta.gif" width="120" alt="wajah cinta">
  <img src="docs/img/lampu_sorot.gif" width="120" alt="mobil lampu_sorot">
  <img src="docs/img/kokpit.gif" width="120" alt="gundam kokpit">
</p>
<p align="center"><sub>Pratinjau kecil klip di firmware (wajah, mobil, gundam). Di perangkat semuanya frame JPEG 240 lebar.</sub></p>

**Firmware 0.7.0** · **MIT © rzmong** · Situs + pemasang browser: **https://rz7mong.github.io/mochi-dfplayer/** (sumber di [`docs/`](docs/))

### 📚 Panduan di situs

| Halaman | Isi |
|---|---|
| ⚡ [Instalasi firmware](https://rz7mong.github.io/mochi-dfplayer/) ([`docs/index.html`](docs/index.html)) | Flash dari browser, setelah flash, kalau gagal |
| 🧩 [Demo rakit](https://rz7mong.github.io/mochi-dfplayer/pemasangan.html) ([`docs/pemasangan.html`](docs/pemasangan.html)) | Komponen → solder tiap modul → kartu SD → flash → uji nyala, langkah demi langkah |
| 🔌 [Diagram kabel](https://rz7mong.github.io/mochi-dfplayer/pemasangan-kabel.html) ([`docs/pemasangan-kabel.html`](docs/pemasangan-kabel.html)) | Diagram lengkap, tabel pin per modul / per GPIO, tips |
| 🎵 [Animasi &amp; suara](https://rz7mong.github.io/mochi-dfplayer/kelola.html) ([`docs/kelola.html`](docs/kelola.html)) | Tambah GIF, suara `/MP3`, pemutar musik `/01`, suara Chronos `/02` `/03` |
| 📖 [Cara pakai](https://rz7mong.github.io/mochi-dfplayer/panduan.html) · 🔧 [Rakit &amp; kartu SD](https://rz7mong.github.io/mochi-dfplayer/instalasi.html) · 🎞️ [Tambah animasi](https://rz7mong.github.io/mochi-dfplayer/animasi.html) | Menu, gerakan, kartu SD, anggaran flash |

## ✨ Fitur

- **14 model animasi** di flash: 6 wajah, 2 mobil, 6 gundam (nama Indonesia). JPEG lebar 240, tempo per klip. Saat nyala langsung `wajah/senyum_kedip`.
- **Suara MP3 per model** dari kartu microSD DFPlayer (`/MP3/0001.mp3` …), **diputar sampai habis** lalu diulang. Tidak lagi dipotong tiap putaran gambar.
- **Pemutar MP3** di menu: putar/jeda, berikut/sebelum, volume, mode *ulang semua / ulang 1 / acak*, layar nomor trek dan volume. Lagu dari folder `/01`, terpisah dari suara animasi. Musik tetap jalan saat kembali ke animasi.
- **Chronos (BLE)**: jam dan tanggal dari HP, baterai HP, notifikasi, panggilan masuk, navigasi, cari perangkat.
- **Satu tombol sentuh**: ketuk, ketuk 2×, tahan, tahan 2 detik (menu). TTP223 tanpa solder: jenis sensor dideteksi otomatis saat nyala.
- **Goyang** (opsional, MPU6050): klip "pusing" tema itu (tema tanpa klip pusing: klip lain di tema yang sama).
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
| 1 | Speaker 8 Ω (4 Ω juga bisa) | 0,5–3 W, ke SPK_1 / SPK_2 |
| 1 | Kartu microSD | FAT32, ≤ 32 GB |
| 1 | Resistor 1 kΩ | jalur GPIO20 → RX DFPlayer |
| 1 | Tombol atau TTP223 | ke GPIO1, tanpa solder; jenis dideteksi saat nyala ([catatan](#catatan-sentuh)) |
| 1 | MPU6050 (GY-521) | opsional, untuk goyang. Tanpa modul ini goyang mati, yang lain jalan |
| 1 | Elko 100–470 µF | disarankan, di VCC–GND DFPlayer (mengurangi letup / reset saat volume keras) |
| 2 | Resistor 4,7 kΩ | opsional, pull-up SDA/SCL ke 3V3 jika modul MPU6050 belum punya |

## 🔌 Kabel

Sumber: [`firmware/include/MochiRzmong.h`](firmware/include/MochiRzmong.h) dan [`User_Setup_ST7789.h`](firmware/include/User_Setup_ST7789.h).

<p align="center"><img src="docs/wiring-dfplayer.svg" width="720" alt="Diagram kabel Mochi DFPlayer"></p>

Rakit langkah demi langkah: [Demo rakit](https://rz7mong.github.io/mochi-dfplayer/pemasangan.html) · tabel lengkap: [Diagram kabel](https://rz7mong.github.io/mochi-dfplayer/pemasangan-kabel.html).

| Kaki modul | ESP32-C3 | Catatan |
|---|---|---|
| TFT SCL / SCLK | GPIO4 | |
| TFT SDA / MOSI | GPIO6 | |
| TFT DC | GPIO3 | |
| TFT RES / RST | GPIO10 | |
| TFT CS | tidak dipakai | modul 8 pin: kaki CS ke GND |
| TFT BLK (lampu latar) | GPIO7 | HIGH nyala, LOW mati saat berhenti |
| TFT VCC | 3V3 | |
| Sentuh / tombol | GPIO1 | polaritas dideteksi otomatis saat nyala |
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

<a id="catatan-sentuh"></a>**Catatan sentuh:** saat nyala firmware membaca GPIO1 ±0,2 detik dengan pull-up lalu pull-down untuk mengenali jenis sensor: TTP223 bawaan pabrik (HIGH saat disentuh), TTP223 dengan pad A disolder (LOW saat disentuh), atau tombol tekan ke GND. Jadi **TTP223 tidak perlu disolder**. Syaratnya: jangan sentuh sensor ±1 detik setelah nyala (TTP223 juga mengkalibrasi diri). Kalau terlanjur dan sensor terbaca tersentuh terus, polaritas dibalik otomatis setelah 10 detik. Paksa manual: `-DMOCHI_TOUCH_ACTIVE_HIGH` (aktif HIGH) atau `-DMOCHI_TOUCH_MODE=2` (aktif LOW + pull-up). Serial monitor menulis jenis yang terdeteksi: `sentuh GPIO1: …`.

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
| 1 | `0019` | wajah/senyum_kedip (klip saat nyala) |
| 2 | `0020` | wajah/pusing (goyang) |
| 3 | `0023` | wajah/cinta (tahan) |
| 4 | `0021` | wajah/sorot |
| 5 | `0022` | wajah/sirine |
| 6 | `0040` | wajah/cinta_pipi |
| 7 | `0034` | mobil/lampu_sorot |
| 8 | `0038` | mobil/speedometer (tahan) |
| 9 | `0026` | gundam/helm_hujan |
| 10 | `0027` | gundam/helm_siaga (tahan) |
| 11 | `0028` | gundam/isyarat |
| 12 | `0029` | gundam/kokpit |
| 13 | `0030` | gundam/kokpit_2 |
| 14 | `0031` | gundam/pilot |
| – | 0001–0018, 0024, 0025, 0032, 0033, 0035–0037, 0039 | tidak dipakai, boleh tidak ada |

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

Sentuhan dalam 0,8 dtk pertama setelah nyala diabaikan; jika sensor sudah tersentuh saat nyala, firmware menunggu dilepas dulu. Tema mobil tidak punya klip goyang/tahan khusus: tahan memakai klip terakhirnya (speedometer), goyang memakai klip berikutnya.

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
| `-DMOCHI_TOUCH_ACTIVE_HIGH` | Paksa sentuh HIGH = ditekan, dengan pull-down (bawaan: deteksi otomatis saat nyala) |
| `-DMOCHI_TOUCH_MODE=2` | Paksa sentuh LOW = ditekan, dengan pull-up (tombol ke GND / TTP223 pad A) |
| `-DMOCHI_DEFAULT_ROTATION=0` | Rotasi awal layar (0–3, bawaan 2 = pin LCD di bawah, tatakan GMT130). Menu "Putar layar" menimpanya |
| `-DMOCHI_BLE_NAME=\"nama\"` | Nama perangkat di aplikasi Chronos |

**Ukuran flash.** App 0x3F0000 (4.128.768 B, partisi terbesar di flash 4 MB). Build 0.7.0: **94,5%** flash (3.900.202 B), RAM 12,4%. Klip JPEG mentah 3,43 MB (14 klip) tidak muat bersama BLE, jadi `embed_jpeg.py` punya **anggaran** (`custom_jpeg_budget = 3300000` di `platformio.ini`). Jika total klip melebihinya, frame yang **nyaris sama** dengan frame sebelumnya (saat ini ≤ 0,3% piksel berbeda, jadi 3,26 MB) dipakai ulang. Jumlah frame dan tempo tetap; file aset tidak diubah. Hasilnya tercetak saat build (`ANGGARAN: …`).

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
2. Daftarkan di `firmware/assets/meta.json` → `"jpeg_clips"`: `["<tema>", "<nama>", "", <trek>]`. Peran: `""` biasa, `"dizzy"` saat goyang, `"heart"` saat tahan. Trek boleh dikosongkan (otomatis nomor berikutnya). Tambahkan di **akhir** `"jpeg_clips"`, jangan di `"builtins"` (nomor trek builtin = posisinya, jadi baris baru akan bentrok dengan trek 19 `wajah/senyum_kedip`). Panduan lengkap: [Animasi & suara](https://rz7mong.github.io/mochi-dfplayer/kelola.html).
3. Build + flash: `pio run -e esp32-c3-dfplayer -t upload`. Perhatikan baris `ANGGARAN` jika klip banyak.
4. `python tools/daftar_trek.py` untuk melihat nomor trek, lalu salin `/MP3/000N.mp3` ke kartu.

Aturan: klip satu tema selalu dikelompokkan. `meta.json` `"theme_order"` mengatur urutan tema (sekarang wajah, mobil, gundam; tema lain di akhir), `"boot_theme"` menentukan model saat nyala (sekarang `wajah`). Pakai hanya media yang boleh kamu sebarkan ([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)).

## 🛠️ Kalau ada masalah

| Gejala | Cek |
|---|---|
| Port tidak muncul | Chrome/Edge di komputer, kabel USB data, tahan BOOT saat colok. Linux: tambahkan user ke grup `dialout`. Tutup monitor serial lain |
| Layar hitam | SCLK 4, MOSI 6, DC 3, RST 10, BLK 7; CS modul 8 pin ke GND. Mode berhenti juga hitam: ketuk 1× |
| Gambar terbalik/miring | Menu → Putar layar, atau `-DMOCHI_DEFAULT_ROTATION` |
| Tidak ada suara | DFPlayer VCC 5 V, GPIO20 →(1 kΩ)→ RX, TX → GPIO21, speaker SPK_1/SPK_2, kartu FAT32, folder `/MP3`. Serial: `DFPlayer menjawab` |
| Suara tidak diulang / lagu tidak lanjut | Kabel TX modul → GPIO21 (pesan "trek selesai"), atau pasang BUSY + `-DMOCHI_PIN_DF_BUSY=5` |
| Pemutar: "folder /01 kosong" | Lagu harus `/01/001.mp3`, `/01/002.mp3`, … |
| Ketukan tidak terbaca / selalu "tahan" | Lihat log `sentuh GPIO1: …` dan [catatan sentuh](#catatan-sentuh). "tombol ke GND / mengambang" padahal TTP223 = kabel I/O putus. VCC TTP223 3V3. Casing di atas TTP223 ≤ 2 mm, tanpa logam |
| Bunyi letup / ESP reset saat suara mulai | Daya USB kurang: kabel/charger lebih baik, kapasitor 100–470 µF di VCC–GND DFPlayer |
| Warna negatif / merah-biru tertukar | `User_Setup_ST7789.h`: `TFT_INVERSION_ON` → `TFT_INVERSION_OFF`, atau `TFT_RGB_ORDER` `TFT_BGR` → `TFT_RGB` |
| Build: `No module named PIL` | `~/.platformio/penv/bin/pip install pillow` (Python milik PlatformIO) |
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

MIT © rzmong untuk kode. Klip wajah, mobil, dan gundam punya catatan hak tersendiri di [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
