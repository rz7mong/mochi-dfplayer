// Mochi DFPlayer 0.6.10 — perilaku pemutar sama pikapet / bangdc90.
// Tema "wajah": senyum_kedip utama (klip awal), pusing goyang, cinta tahan. Lalu mobil, lalu gundam (theme_order).
// Semua klip JPEG penuh 240 lebar (gundam 240x240 dari paket rzmong, mobil dan wajah/cinta_pipi 240x120 di tengah). Tempo dan nomor trek per klip (jpeg_clips.h).
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <math.h>
#include "jpeg_clips.h"
#include <MochiDfPlayer.h>
#include "MochiRzmong.h"

TFT_eSPI tft;

static const float SHAKE_G = 1.2f;
static const uint32_t SHAKE_WINDOW_MS = 1000;
static const uint32_t SHAKE_COOLDOWN_MS = 1000;

enum class Mode { Stopped, Playing };

static Mode mode = Mode::Stopped;
static int model = 0;
static int clip = 0;
static int frame = 0;
static int savedFrame = 0;
static bool shakeClip = false;
static bool holdClip = false;
static uint32_t lastFrame = 0;
static bool mpuOk = false;
static float lastMag = 1.0f;
static int shakeCount = 0;
static uint32_t shakeWindow = 0;
static uint32_t lastShake = 0;

static int themeStart(int i) {
  if (i < 0) i = 0;
  if (i >= JPEG_CLIP_COUNT) i = JPEG_CLIP_COUNT - 1;
  const char *theme = JPEG_CLIPS[i].theme;
  int s = i;
  while (s > 0 && strcmp(JPEG_CLIPS[s - 1].theme, theme) == 0) s--;
  return s;
}
static int themeEnd(int i) {
  int s = themeStart(i);
  int e = s;
  while (e + 1 < JPEG_CLIP_COUNT && strcmp(JPEG_CLIPS[e + 1].theme, JPEG_CLIPS[s].theme) == 0) e++;
  return e;
}
static int roleClip(uint8_t role) {
  for (int i = themeStart(model), e = themeEnd(model); i <= e; i++)
    if (JPEG_CLIPS[i].role == role) return i;
  return -1;
}
static int mainClip() { return model; }
static int dizzyClip() {
  int r = roleClip(JPEG_ROLE_DIZZY);
  if (r >= 0) return r;
  int s = themeStart(model), e = themeEnd(model);
  return e > s ? s + 1 + ((model - s) % (e - s)) : model;
}
static int heartClip() {
  int r = roleClip(JPEG_ROLE_HEART);
  return r >= 0 ? r : themeEnd(model);
}
static int bootModel() {
  for (int i = 0; i < JPEG_CLIP_COUNT; i++)
    if (strcmp(JPEG_CLIPS[i].theme, JPEG_BOOT_THEME) == 0) return i;
  return 0;
}

static void backlight(bool on) { digitalWrite(MOCHI_PIN_TFT_BL, on ? HIGH : LOW); }

static int drawnClip = -1;
static const uint8_t *drawnJpg = nullptr;

static bool drawFrame(int c, int f) {
  if (c < 0 || c >= JPEG_CLIP_COUNT) return false;
  const JpegClip &clipInfo = JPEG_CLIPS[c];
  if (f < 0 || f >= clipInfo.n) return false;
  if (c != drawnClip) {
    // Klip lebih kecil dari layar (mis. 160x80 dasai) digambar di tengah, sisanya hitam.
    if (clipInfo.x || clipInfo.y) tft.fillScreen(TFT_BLACK);
    drawnClip = c;
    drawnJpg = nullptr;
  }
  if (clipInfo.frames[f] == drawnJpg) return true;  // frame tahan: gambar sama, tidak perlu decode ulang
  drawnJpg = clipInfo.frames[f];
  return TJpgDec.drawJpg(clipInfo.x, clipInfo.y, clipInfo.frames[f], clipInfo.sizes[f]) == JDR_OK;
}

static void playAudio(int c) {
  mochiDfPlayTrack(JPEG_CLIPS[c].track);
}

static void stopAll() {
  mochiDfStop();
  tft.fillScreen(TFT_BLACK);
  drawnClip = -1;
  backlight(false);
  mode = Mode::Stopped;
  shakeClip = false;
  holdClip = false;
}

static void startMain() {
  if (model < 0 || model >= JPEG_CLIP_COUNT) model = 0;
  clip = mainClip();
  frame = 0;
  savedFrame = 0;
  shakeClip = false;
  holdClip = false;
  mode = Mode::Playing;
  lastFrame = millis();
  backlight(true);
  playAudio(clip);
  Serial.printf("model %d/%d %s/%s trek %d\n", model + 1, JPEG_CLIP_COUNT, JPEG_CLIPS[model].theme, JPEG_CLIPS[model].stem,
                JPEG_CLIPS[model].track);
}
static void nextModel() {
  model = (model + 1) % JPEG_CLIP_COUNT;
  startMain();
}

static void mpuInit() {
  Wire.begin(MOCHI_PIN_MPU_SDA, MOCHI_PIN_MPU_SCL);
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 tidak ada");
    return;
  }
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.write(0x1A);
  Wire.write(0x03);
  Wire.endTransmission();
  mpuOk = true;
  Serial.println("MPU6050 siap");
}

static bool shakeNow() {
  if (!mpuOk) return false;
  static uint32_t lastRead = 0;
  uint32_t now = millis();
  if (now - lastRead < 20) return false;
  lastRead = now;
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(0x68, 6);
  if (Wire.available() < 6) return false;
  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  float mag = sqrtf((ax / 16384.0f) * (ax / 16384.0f) + (ay / 16384.0f) * (ay / 16384.0f) + (az / 16384.0f) * (az / 16384.0f));
  float delta = fabsf(mag - lastMag);
  lastMag = mag;
  if (now - lastShake < SHAKE_COOLDOWN_MS) return false;
  if (delta > SHAKE_G) {
    if (shakeCount == 0) shakeWindow = now;
    shakeCount++;
    if (shakeCount >= 3 && now - shakeWindow <= SHAKE_WINDOW_MS) {
      shakeCount = 0;
      lastShake = now;
      return true;
    }
  }
  if (shakeCount && now - shakeWindow > SHAKE_WINDOW_MS) shakeCount = 0;
  return false;
}

// ---------- Sensor sentuh (GPIO1) ----------
// Bawaan: deteksi otomatis saat nyala. Pin dibaca dengan pull-up lalu pull-down:
//   pull-up HIGH, pull-down LOW  -> pin mengambang = tombol ke GND (atau belum tersambung): aktif LOW, pull-up.
//   keduanya LOW                 -> modul mendorong LOW saat diam = TTP223 standar: aktif HIGH, pull-down.
//   keduanya HIGH                -> modul mendorong HIGH saat diam = TTP223 pad A disolder: aktif LOW.
// Paksa lewat build_flags: -DMOCHI_TOUCH_MODE=1 (aktif HIGH) atau 2 (aktif LOW + pull-up). 0 = otomatis.
#ifndef MOCHI_TOUCH_MODE
#define MOCHI_TOUCH_MODE 0
#endif
static const uint32_t TOUCH_DEBOUNCE_MS = 40;
static const uint32_t HOLD_MS = 700;           // tahan >= 0,7 s -> klip tahan
static const uint32_t DOUBLE_TAP_MS = 400;     // lepas pertama -> sentuh kedua maks 0,4 s -> model berikutnya
static const uint32_t TOUCH_BOOT_IGNORE_MS = 800;
static const uint32_t TOUCH_STUCK_MS = 10000;  // TTP223 "tersentuh" terus 10 s -> polaritas dibalik (jari di sensor saat nyala)

static bool touchActiveHigh = true;
static bool touchDriven = true;  // modul mendorong pin (TTP223). Tombol/mengambang: polaritas tidak pernah dibalik
static bool touchRaw = false;
static uint32_t touchRawAt = 0;
static uint32_t touchReadyAt = 0;
static bool waitRelease = false;
static bool pressed = false;
static bool holdFired = false;
static bool held = false;
static uint32_t pressAt = 0;
static uint32_t releaseAt = 0;
static uint8_t taps = 0;

static void touchPinMode() { pinMode(MOCHI_PIN_TOUCH, touchActiveHigh ? INPUT_PULLDOWN : INPUT_PULLUP); }
static bool touchActive() { return (digitalRead(MOCHI_PIN_TOUCH) == HIGH) == touchActiveHigh; }

static bool sampleHigh(uint8_t mode) {
  pinMode(MOCHI_PIN_TOUCH, mode);
  delay(5);
  int hi = 0;
  for (int i = 0; i < 50; i++) {
    hi += digitalRead(MOCHI_PIN_TOUCH) == HIGH;
    delay(2);
  }
  return hi > 25;
}

static void touchInit() {
#if MOCHI_TOUCH_MODE == 1
  touchActiveHigh = true;
  touchDriven = false;
  const char *kind = "dipaksa aktif HIGH";
#elif MOCHI_TOUCH_MODE == 2
  touchActiveHigh = false;
  touchDriven = false;
  const char *kind = "dipaksa aktif LOW";
#else
  bool up = sampleHigh(INPUT_PULLUP);
  bool down = sampleHigh(INPUT_PULLDOWN);
  const char *kind;
  if (up && !down) { touchActiveHigh = false; touchDriven = false; kind = "tombol ke GND / mengambang, aktif LOW"; }
  else if (!up && !down) { touchActiveHigh = true; kind = "TTP223 standar, aktif HIGH"; }
  else if (up && down) { touchActiveHigh = false; kind = "TTP223 pad A, aktif LOW"; }
  else { touchActiveHigh = true; kind = "tidak jelas, aktif HIGH"; }
#endif
  touchPinMode();
  delay(2);
  touchRaw = touchActive();
  touchRawAt = millis();
  touchReadyAt = millis() + TOUCH_BOOT_IGNORE_MS;
  waitRelease = touchRaw;
  Serial.printf("sentuh GPIO%d: %s\n", MOCHI_PIN_TOUCH, kind);
}

static void startHold(uint32_t now) {
  held = true;
  savedFrame = frame;
  clip = heartClip();
  frame = 0;
  holdClip = true;
  lastFrame = now;
  playAudio(clip);
}
static void endHold() {
  held = false;
  holdClip = false;
  clip = mainClip();
  frame = savedFrame;
  playAudio(clip);
}

static void readTouch() {
  uint32_t now = millis();
  bool raw = touchActive();
  if (raw != touchRaw) { touchRaw = raw; touchRawAt = now; }
  if (raw && touchDriven && now - touchRawAt >= TOUCH_STUCK_MS) {
    // TTP223 "tersentuh" terus 10 s: hampir pasti salah deteksi (jari di sensor saat nyala). Balik polaritas.
    touchActiveHigh = !touchActiveHigh;
    touchPinMode();
    if (held) endHold();
    pressed = false;
    holdFired = false;
    taps = 0;
    waitRelease = false;
    touchRaw = touchActive();
    touchRawAt = now;
    Serial.printf("sentuh: aktif terus 10 s, polaritas dibalik ke aktif %s\n", touchActiveHigh ? "HIGH" : "LOW");
    return;
  }
  if ((int32_t)(now - touchReadyAt) < 0) return;      // abaikan sesaat setelah nyala (TTP223 kalibrasi)
  if (now - touchRawAt < TOUCH_DEBOUNCE_MS) return;   // belum stabil
  if (waitRelease) {                                   // tersentuh sejak nyala: tunggu dilepas dulu
    if (!raw) waitRelease = false;
    return;
  }
  if (raw != pressed) {
    pressed = raw;
    if (pressed) {
      pressAt = now;
      holdFired = false;
    } else if (held) {
      endHold();
    } else if (!holdFired) {
      if (++taps >= 2) { taps = 0; nextModel(); }
      else releaseAt = now;
    }
  }
  if (!pressed) return;
  if (!holdFired && now - pressAt >= HOLD_MS) {        // sekali per sentuhan, tidak berulang selama ditahan
    holdFired = true;
    taps = 0;
    if (mode == Mode::Playing && !shakeClip) startHold(now);
  }
}

static void touchTapTimeout() {
  if (taps == 1 && !pressed && !touchRaw && millis() - releaseAt >= DOUBLE_TAP_MS) {  // ketuk tunggal
    taps = 0;
    if (mode == Mode::Playing) stopAll();
    else startMain();
  }
}

void setup() {
  pinMode(MOCHI_PIN_TFT_BL, OUTPUT);
  digitalWrite(MOCHI_PIN_TFT_BL, HIGH);
  Serial.begin(115200);
  touchInit();
  SPI.begin(MOCHI_PIN_TFT_SCLK, -1, MOCHI_PIN_TFT_MOSI, -1);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback([](int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) -> bool {
    if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
  });
  mochiDfInit();
  mochiDfSetVolume(20, true);
  mpuInit();
  model = bootModel();
  startMain();
  Serial.printf("edisi Indonesia, %d model\n", JPEG_CLIP_COUNT);
}

void loop() {
  readTouch();
  touchTapTimeout();
  if (mode == Mode::Playing && !holdClip && !shakeClip && shakeNow()) {
    savedFrame = frame;
    clip = dizzyClip();
    frame = 0;
    shakeClip = true;
    lastFrame = millis();
    playAudio(clip);
  }
  if (mode != Mode::Playing) return;
  if (millis() - lastFrame < JPEG_CLIPS[clip].delay) return;
  lastFrame = millis();
  drawFrame(clip, frame);
  frame++;
  int n = JPEG_CLIPS[clip].n;
  if (frame >= n) {
    if (shakeClip) {
      shakeClip = false;
      clip = mainClip();
      frame = savedFrame;
      playAudio(clip);
    } else {
      frame = 0;
      playAudio(clip);
    }
  }
}
