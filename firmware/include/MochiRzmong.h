#pragma once
/* Konstanta Mochi DFPlayer. Satu-satunya sumber versi: MOCHI_VERSION. */
#define MOCHI_BRAND   "rzmong"
#define MOCHI_VERSION "0.7.1"
#ifndef MOCHI_BLE_NAME            /* nama perangkat di aplikasi Chronos */
#define MOCHI_BLE_NAME "rzmong dfplayer"
#endif
/* Suara Chronos: MP3 di root kartu, nomor URUTAN SALIN (salin 0001..0048 berurutan, baru folder /01). */
#define TRK_NOTIF    41  /* notifikasi */
#define TRK_NAV      42  /* instruksi navigasi baru */
#define TRK_CALL     43  /* panggilan masuk (diulang) */
#define TRK_FIND     44  /* cari perangkat (diulang, volume 30) */
#define TRK_ALARM    45  /* alarm (diulang) */
#define TRK_CONNECT  46  /* Chronos tersambung */
#define TRK_DISCONN  47  /* Chronos terputus */
#define TRK_NAV_END  48  /* navigasi selesai */
/* v0.7.1: semua trek (animasi + Chronos) diputar menurut urutan salin di root (bawaan; -DMOCHI_DF_MP3_FOLDER = /MP3).
 *         Chronos: cari perangkat, alarm, cuaca + musik HP di halaman jam (ketuk putar/jeda, 2x lagu berikut),
 *         notifikasi dibuka klip cinta_pipi, suara sambung/putus/navigasi selesai, overlay ditutup dengan ketuk.
 *         Tiap animasi punya suara sendiri (sd/mp3, dari aset rzmong mochi-themes).
 * v0.7.0: Chronos BLE (jam, baterai HP, notifikasi, panggilan, navigasi), menu LCD, pemutar MP3 folder /01,
 *         trek animasi /MP3/000N.mp3 menurut nama dan diputar sampai habis, versi tunggal, rotasi dari header.
 *         Sentuh: polaritas dideteksi otomatis saat nyala (dari v0.6.10), MOCHI_TOUCH_ACTIVE_HIGH tetap sebagai paksaan.
 *         Klip dari v0.6.9/0.6.8: nama Indonesia (wajah, mobil, gundam), theme_order, boot wajah/senyum_kedip.
 * v0.6.10: sensor sentuh deteksi otomatis (TTP223 standar, pad A, atau tombol ke GND).
 * v0.6.9: nama animasi Indonesia, theme_order, boot wajah/senyum_kedip.
 * v0.6.8: 6 gundam, 5 mochi, 3 dasai; dasai semua frame q80.
 * v0.6.6: gundam 12 GIF rzmong, 240x240 (PR #1).
 * v0.6.5: gundam 8 adegan 240x240, dasai tinggal video03, video07, video2.
 * v0.6.2-0.6.4: tema mochi, klip JPEG penuh, tempo/trek per klip, app 0x3F0000 tanpa spiffs.
 * v0.6.1: dokumen dan pesan edisi bahasa Indonesia.
 * v0.6.0: 18 model JPEG, ketuk dua kali ganti model.
 * v0.5.7-0.5.9: pinout pikapet / bangdc90 (DC 3, RST 10, BL 7, MPU6050 8/9), DFPlayer UART 20/21.
 */
#ifndef MOCHI_DEFAULT_ROTATION    /* 2 = LCD pin di bawah (tatakan GMT130). Pin di atas / tegak pikapet: -DMOCHI_DEFAULT_ROTATION=0 */
#define MOCHI_DEFAULT_ROTATION 2
#endif
#define MOCHI_ROT_LAYOUT 1  /* versi arah pasang LCD di NVS ("rotv"); naikkan jika default rotasi berubah lagi */
#define MOCHI_PIN_TOUCH 1  /* bukan pin strap (strap C3: GPIO2, 8, 9); polaritas dideteksi saat nyala, lihat main.cpp */
#define MOCHI_PIN_TFT_SCLK 4
#define MOCHI_PIN_TFT_MOSI 6
#define MOCHI_PIN_TFT_CS -1
#define MOCHI_PIN_TFT_DC 3
#define MOCHI_PIN_TFT_RST 10
#define MOCHI_PIN_TFT_BL 7
/* DFPlayer: ESP TX GPIO20 -> RX modul lewat 1k. ESP RX GPIO21 <- TX modul. */
#define MOCHI_PIN_DF_TX 20
#define MOCHI_PIN_DF_RX 21
/* BUSY DFPlayer (LOW saat memutar). Opsional: -DMOCHI_PIN_DF_BUSY=5. -1 = tidak disambung. */
#ifndef MOCHI_PIN_DF_BUSY
#define MOCHI_PIN_DF_BUSY -1
#endif
/* MPU6050 sama pikapet: SDA GPIO8, SCL GPIO9. GPIO9 strap, jangan ditarik LOW saat boot. */
#define MOCHI_PIN_MPU_SDA 8
#define MOCHI_PIN_MPU_SCL 9
