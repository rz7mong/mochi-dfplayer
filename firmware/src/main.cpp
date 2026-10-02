// Mochi DFPlayer 0.5.9 — perilaku pemutar sama pikapet / bangdc90.
// Aset JPEG tetap milik repo ini, bukan frame video mereka.
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

static const uint8_t FRAME_MS = 100;
static const uint8_t HOLD_MS = 400;
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
static uint32_t shortAt = 0;
static uint32_t lastFrame = 0;
static bool mpuOk = false;
static float lastMag = 1.0f;
static int shakeCount = 0;
static uint32_t shakeWindow = 0;
static uint32_t lastShake = 0;
static int lastBtn = HIGH;
static uint32_t debounceAt = 0;
static bool pressed = false;
static uint32_t pressAt = 0;
static bool held = false;

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
static int mainClip() { return model; }
static int dizzyClip() {
  int s = themeStart(model), e = themeEnd(model);
  return e > s ? s + 1 + ((model - s) % (e - s)) : model;
}
static int heartClip() { return themeEnd(model); }

static void backlight(bool on) { digitalWrite(MOCHI_PIN_TFT_BL, on ? HIGH : LOW); }

static bool drawFrame(int c, int f) {
  if (c < 0 || c >= JPEG_CLIP_COUNT) return false;
  const JpegClip &clipInfo = JPEG_CLIPS[c];
  if (f < 0 || f >= clipInfo.n) return false;
  return TJpgDec.drawJpg(0, 0, clipInfo.frames[f], clipInfo.sizes[f]) == JDR_OK;
}

static void playAudio(int c) {
  uint8_t track = (uint8_t)(c + 1);
  mochiDfPlayTrack(track);
}

static void stopAll() {
  mochiDfStop();
  tft.fillScreen(TFT_BLACK);
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
  Serial.printf("model %d/%d %s/%s\n", model + 1, JPEG_CLIP_COUNT, JPEG_CLIPS[model].theme, JPEG_CLIPS[model].stem);
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

static void readButton() {
  bool down = digitalRead(MOCHI_PIN_TOUCH) == LOW;
  uint32_t now = millis();
  if (down != pressed) debounceAt = now;
  if (now - debounceAt < 15) return;
  pressed = down;
  if (pressed) {
    if (!held && pressAt == 0) pressAt = now;
    if (!held && now - pressAt >= HOLD_MS && mode == Mode::Playing && !shakeClip) {
      held = true;
      savedFrame = frame;
      clip = heartClip();
      frame = 0;
      holdClip = true;
      lastFrame = now;
      playAudio(clip);
    }
  } else {
    if (held) {
      held = false;
      pressAt = 0;
      holdClip = false;
      clip = mainClip();
      frame = savedFrame;
      playAudio(clip);
    } else if (pressAt && now - pressAt < HOLD_MS) {
      pressAt = 0;
      if (shortAt && now - shortAt < 350) {
        shortAt = 0;
        nextModel();
      } else shortAt = now;
    } else pressAt = 0;
  }
}

void setup() {
  pinMode(MOCHI_PIN_TFT_BL, OUTPUT);
  digitalWrite(MOCHI_PIN_TFT_BL, HIGH);
  Serial.begin(115200);
  pinMode(MOCHI_PIN_TOUCH, INPUT_PULLUP);
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
  startMain();
  Serial.printf("pikapet mode, %d model\n", JPEG_CLIP_COUNT);
}

void loop() {
  readButton();
  if (shortAt && millis() - shortAt >= 350 && !pressed) {
    shortAt = 0;
    if (mode == Mode::Playing) stopAll();
    else startMain();
  }
  if (mode == Mode::Playing && !holdClip && !shakeClip && shakeNow()) {
    savedFrame = frame;
    clip = dizzyClip();
    frame = 0;
    shakeClip = true;
    lastFrame = millis();
    playAudio(clip);
  }
  if (mode != Mode::Playing) return;
  if (millis() - lastFrame < FRAME_MS) return;
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
