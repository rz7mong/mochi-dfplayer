# Third-party notices

This project’s **firmware code** is MIT (see [LICENSE](LICENSE)). The following libraries are used as dependencies and retain their own licenses.

## TFT_eSPI

- Project: [Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- Typical license: **MIT** (see upstream `License.txt` / repository)
- Use: ST7789 SPI display driver

## ChronosESP32 dan ESP32Time

- Project: [fbiego/ChronosESP32](https://github.com/fbiego/ChronosESP32) (1.9.1), [fbiego/ESP32Time](https://github.com/fbiego/ESP32Time)
- License: **MIT**
- Use: Chronos BLE (jam, baterai HP, notifikasi, panggilan, navigasi)

## NimBLE-Arduino

- Project: [h2zero/NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) (dependensi ChronosESP32)
- License: **Apache-2.0**
- Use: tumpukan BLE

## DFRobotDFPlayerMini

- Project: [DFRobot/DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini) (1.0.6)
- License: **GNU LGPL** (lihat header pustaka)
- Use: perintah UART DFPlayer Mini

## TJpg_Decoder

- Project: [Bodmer/TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) (memuat TJpgDec karya ChaN)
- License: lihat `license.txt` upstream (TJpgDec: lisensi bebas ala BSD)
- Use: decode frame JPEG animasi

## Espressif / Arduino-ESP32

- Platform and core provided by Espressif / Arduino-ESP32
- Licenses: see Espressif and Arduino-ESP32 documentation

## Media assets (GIF / JPEG / MP3)

Code license **does not** automatically cover media inside Release `assets-v1` or files you place on the SD card.

- Only distribute GIF/SFX that you **created** or that you have **rights** to redistribute.
- Do **not** package assets taken from commercial products (including Dasai or similar brands) unless you hold a license.

If you fork this repo, replace theme packs with your own media before publishing.

## Klip JPEG tema wajah, mobil, gundam

`firmware/assets/builtin/jpeg/wajah/` senyum_kedip, pusing, cinta, sorot, sirine (dulu mochi/full1, chongmat1, xoadau1, video17, video18) ditambahkan pemilik repo dari berkasnya sendiri. Klip ini tidak termasuk lisensi MIT kode. Pastikan hak distribusinya sebelum dibagikan ulang.

- `mobil/` lampu_sorot, speedometer dan `wajah/cinta_pipi` (dulu dasai/video03, video07, video2): dikonversi dari header `video*.h` di [bangdc90/dasai_mochi_tft](https://github.com/bangdc90/dasai_mochi_tft). Repo itu tidak mencantumkan lisensi; tampaknya diambil dari video produk Dasai Mochi. Pastikan izin sebelum distribusi ulang.
- `gundam/` (helm_hujan, helm_siaga, isyarat, kokpit, kokpit_2, pilot): dikonversi dari `gif/gundam/` di rilis `assets-v1` repo rz7mong/mochi-rzmong milik pemilik repo.

Pratinjau `docs/img/*.gif` (120×120) dibuat dari klip di atas dan mengikuti catatan yang sama.

## Suara kartu SD (`sd/mp3/`)

- Semua 48 file di `sd/mp3/` (14 suara animasi 0019–0040, suara Chronos 0041–0048, pengisi hening) **dibuat dari nol** oleh [`sd/buat_suara.py`](sd/buat_suara.py) dengan numpy (osilator, derau terfilter, amplop) lalu dienkode ffmpeg/LAME. Tidak ada sampel pihak ketiga. Lisensi: **MIT**, sama dengan kode.
- `sd/buat_suara.py --pack mochi-themes.zip` (opsional) memakai WAV dari rilis [`assets-v1`](https://github.com/rz7mong/mochi-rzmong/releases/tag/assets-v1) repo rz7mong/mochi-rzmong milik pemilik repo (`gundam/*.wav`, `polisi/police.wav`, `mobil/headlights|car|accel|speed_3|revs.wav`, `wajah/happy|squint|smile|look_*|distracted|confused_2|love_hearts_kiss|embarrassed|uwu.wav`). Paket itu tidak mencantumkan lisensi, jadi hasilnya **tidak** disertakan di repo ini. Pastikan hak Anda sebelum menyebarkannya.
- Repo [bangdc90/dasai_mochi_tft](https://github.com/bangdc90/dasai_mochi_tft) dicek: tidak berisi berkas suara (mp3/wav) untuk klip mobil/cinta_pipi.
