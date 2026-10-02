#pragma once
#define MOCHI_BRAND   "rzmong"
#define MOCHI_WATERMARK MOCHI_BRAND
#define MOCHI_AP_NAME "rzmong mochi"
#ifndef MOCHI_AP_PASS  /* override: build_flags = -DMOCHI_AP_PASS=\"sandibaru123\" */
#define MOCHI_AP_PASS "rzmong123"
#endif
#define MOCHI_VERSION "0.6.0"
/* v0.6.0: 18 model JPEG milik repo, ketuk dua kali ganti model.\n * v0.5.9: perilaku pemutar disamakan dengan pikapet.
 * v0.5.8: backlight GPIO7 seperti pikapet, HIGH nyala, LOW mati saat jeda.
 * v0.5.7: pinout disamakan dengan Dasai Mochi pikapet / bangdc90: TFT DC GPIO3, RST GPIO10, CS tidak dipakai,
 *         backlight GPIO7, MPU6050 SDA GPIO8 / SCL GPIO9. DFPlayer tetap UART GPIO20/21.
 * v0.5.6: 30 built-in GIF+WAV slots in flash (new happy-blink wajah/default from video, colourful wajah/musik/mobil + gundam),
 *         built-in WAV played from flash, 11 tap reacts, clock honours GIF frame delay, default between idle clips.
 * v0.5.5: embedded wajah/default.gif for Jam HP; shared SPI init before TFT; upload preempts SD playback.
 * v0.5.4: default rotation 2 (180 deg) for the GMT130 LCD mounted pins-down in case/tatakan_GMT130_fit.stl.
 * AP name/pass unchanged. Override pass via build_flags only.
 */
#ifndef MOCHI_DEFAULT_ROTATION  /* 2 = LCD pin di bawah (tatakan GMT130). Tatakan lama / pin di atas: build_flags = -DMOCHI_DEFAULT_ROTATION=0 */
#define MOCHI_DEFAULT_ROTATION 2
#endif
#define MOCHI_ROT_LAYOUT 1  /* versi arah pasang LCD di NVS ("rotv"); naikkan jika default rotasi berubah lagi */
#define MOCHI_PIN_TOUCH 1
#define MOCHI_PIN_TFT_SCLK 4
#define MOCHI_PIN_TFT_MOSI 6
#define MOCHI_PIN_TFT_CS -1
#define MOCHI_PIN_TFT_DC 3
#define MOCHI_PIN_TFT_RST 10
#define MOCHI_PIN_TFT_BL 7
/* DFPlayer: ESP TX GPIO20 -> RX modul lewat 1k. ESP RX GPIO21 <- TX modul. */
#define MOCHI_PIN_DF_TX 20
#define MOCHI_PIN_DF_RX 21
/* MPU6050 sama pikapet: SDA GPIO8, SCL GPIO9. GPIO9 strap, jangan ditarik LOW saat boot. */
#define MOCHI_PIN_MPU_SDA 8
#define MOCHI_PIN_MPU_SCL 9
#define MOCHI_PIN_SD_SCK MOCHI_PIN_TFT_SCLK
#define MOCHI_PIN_SD_MISO -1
#define MOCHI_PIN_SD_MOSI MOCHI_PIN_TFT_MOSI
#define MOCHI_PIN_SD_CS -1
#define MOCHI_PIN_I2S_BCLK MOCHI_PIN_DF_RX
#define MOCHI_PIN_I2S_LRC MOCHI_PIN_DF_TX
#define MOCHI_PIN_I2S_DIN 8
struct MochiPart { const char *theme; const char *stem; };
struct MochiReact { const char *name; const char *theme; const char *stem; };
static const MochiReact MOCHI_REACT[] = {
  {"tickle","wajah","raspberry"},
  {"kedip","wajah","squint"},
  {"cinta","wajah","love_hearts_kiss"},
  {"marah","wajah","angry_2"},
  {"ketawa","wajah","smirk"},
  {"ngantuk","wajah","sleepy"},
  {"nguap","wajah","yawn_tired"},
  {"pelangi","musik","rainbow"},
  {"pong","musik","pong"},
  {"ngebut","mobil","revs"},
  {"hadouken","gundam","hadouken_hit"}
};
static const int MOCHI_REACT_COUNT = sizeof(MOCHI_REACT)/sizeof(MOCHI_REACT[0]);
static const char *MOCHI_THEMES[] = {
  "wajah","gundam","mobil","polisi","musik","neon","anime","makanan","intro"
};
static const int MOCHI_THEME_COUNT = 9;
