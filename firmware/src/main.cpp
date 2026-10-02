// Mochi DFPlayer — versi di MOCHI_VERSION (include/MochiRzmong.h).
// Animasi JPEG penuh di flash (jpeg_clips.h), suara DFPlayer Mini, Chronos BLE, menu LCD satu tombol, pemutar MP3.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <Preferences.h>
#include <ChronosESP32.h>
#include <math.h>
#include "jpeg_clips.h"
#include <MochiDfPlayer.h>
#include "MochiRzmong.h"

TFT_eSPI tft;
ChronosESP32 watch(MOCHI_BLE_NAME, CF_ESP32_240x240);
Preferences prefs;

// Warna sama dengan mochi-rzmong.
static const uint16_t C_BG = 0x1082, C_BAR = 0xFD20, C_SEL = 0xFE60, C_TEXT = 0xEF7D, C_DIM = 0x8410;
static const uint16_t C_RED = 0xF985, C_TEAL = 0x07F4, C_BLUE = 0x3C7F, C_PINK = 0xF81F, C_YEL = 0xFFE0;

// ---------- waktu sentuh ----------
static const uint16_t DEBOUNCE_MS = 15;
static const uint16_t TAP_GAP_MS = 350;   // jeda maks antar-ketukan untuk ketuk 2x
static const uint16_t HOLD_MS = 400;      // tahan
static const uint16_t LONG_MS = 2000;     // tahan lama = menu / kembali
static const float SHAKE_G = 1.2f;
static const uint32_t SHAKE_WINDOW_MS = 1000;
static const uint32_t SHAKE_COOLDOWN_MS = 1000;
static const uint32_t TOUCH_BOOT_IGNORE_MS = 800;   // TTP223 kalibrasi sesaat setelah nyala
static const uint32_t TOUCH_STUCK_MS = 10000;       // modul "tersentuh" terus 10 dtk -> polaritas dibalik
static const uint32_t TOUCH_FLIP_WINDOW_MS = 60000; // pembalikan hanya 60 dtk pertama (salah deteksi saat nyala)
// Polaritas sensor: bawaan dideteksi otomatis saat nyala (lihat touchInit).
// Paksa lewat build_flags: -DMOCHI_TOUCH_ACTIVE_HIGH (TTP223 standar), atau -DMOCHI_TOUCH_MODE=1 (aktif HIGH)
// / 2 (aktif LOW + pull-up, tombol ke GND / TTP223 pad A). 0 = otomatis.
#ifndef MOCHI_TOUCH_MODE
#ifdef MOCHI_TOUCH_ACTIVE_HIGH
#define MOCHI_TOUCH_MODE 1
#else
#define MOCHI_TOUCH_MODE 0
#endif
#endif

// ---------- pengaturan (NVS) ----------
static int volume = 28;                   // 0..30, sama untuk animasi dan musik
static int rot = MOCHI_DEFAULT_ROTATION;
bool chronosOn = false, clockOn = false, chronosNav = true;

bool uiDirty = true;
static int clockDrawn = -1;

// ---------- UI ----------
enum class Ui : uint8_t { Anim, Clock, Menu, Player };
static Ui ui = Ui::Anim;
static Ui homeUi() { return clockOn ? Ui::Clock : Ui::Anim; }

// ---------- suara ----------
enum class Owner : uint8_t { None, Anim, Music, Notif, Ring };
static Owner dfOwner = Owner::None;
enum class MState : uint8_t { Stopped, Playing, Paused };
static MState mState = MState::Stopped;
static int mTrack = 1, mCount = -1;
static uint8_t mMode = 0;                 // 0 ulang semua, 1 ulang 1, 2 acak
static const char *const MODE_NAME[3] = {"Ulang semua", "Ulang 1", "Acak"};
static uint32_t mLastCmd = 0;
static bool mEmpty = false;               // folder /01 kosong / tidak ada
static uint32_t notifSoundAt = 0;
static bool musicSession() { return mState != MState::Stopped; }

bool overlayActive();
extern bool chronoConn;

static void savePrefs() {
  prefs.putUChar("vol", volume);
  prefs.putUChar("rot", rot);
  prefs.putBool("chrono", chronosOn);
  prefs.putBool("clock", clockOn);
  prefs.putBool("nav", chronosNav);
  prefs.putUChar("mmode", mMode);
  prefs.putUShort("mtrack", mTrack);
}
static void loadPrefs() {
  volume = prefs.getUChar("vol", 28);
  if (volume > 30) volume = 28;
  rot = prefs.getUChar("rot", MOCHI_DEFAULT_ROTATION) & 3;
  if (prefs.getUChar("rotv", 0) != MOCHI_ROT_LAYOUT) {  // default rotasi berubah: pakai default baru sekali
    rot = MOCHI_DEFAULT_ROTATION;
    prefs.putUChar("rot", rot);
    prefs.putUChar("rotv", MOCHI_ROT_LAYOUT);
  }
  chronosOn = prefs.getBool("chrono", false);
  clockOn = prefs.getBool("clock", false);
  chronosNav = prefs.getBool("nav", true);
  mMode = prefs.getUChar("mmode", 0) % 3;
  mTrack = prefs.getUShort("mtrack", 1);
  if (mTrack < 1 || mTrack > 255) mTrack = 1;
}

static void backlight(bool on) { digitalWrite(MOCHI_PIN_TFT_BL, on ? HIGH : LOW); }

// =====================================================================
// Sentuh: satu input -> ketuk 1x / 2x, tahan (0,4 dtk), tahan lama (2 dtk)
// =====================================================================
enum class Ev : uint8_t { None, Tap1, Tap2, HoldStart, HoldEnd, Long };
static bool touchActiveHigh = true;
static bool touchDriven = true;           // modul mendorong pin (TTP223). Tombol / mengambang: polaritas tidak dibalik
static uint32_t touchReadyAt = 0;
static uint32_t touchBootAt = 0;
static bool waitRelease = false;          // tersentuh saat nyala: tunggu dilepas dulu
static int lastRaw = 0;
static uint32_t debounceAt = 0;
static bool pressed = false, holdFired = false, longFired = false;
static uint32_t pressAt = 0, lastTapAt = 0;
static uint8_t taps = 0;
static bool multiTap = true;              // false = ketuk langsung dieksekusi (tanpa menunggu ketuk 2x)

static void touchPinMode() { pinMode(MOCHI_PIN_TOUCH, touchActiveHigh ? INPUT_PULLDOWN : INPUT_PULLUP); }
static int touchRawRead() { return (digitalRead(MOCHI_PIN_TOUCH) == HIGH) == touchActiveHigh ? 1 : 0; }

static bool sampleHigh(uint8_t pm) {
  pinMode(MOCHI_PIN_TOUCH, pm);
  delay(5);
  int hi = 0;
  for (int i = 0; i < 50; i++) { hi += digitalRead(MOCHI_PIN_TOUCH) == HIGH; delay(2); }
  return hi > 25;
}

// Deteksi otomatis saat nyala: pin dibaca dengan pull-up lalu pull-down.
//   pull-up HIGH, pull-down LOW -> mengambang = tombol ke GND (atau belum tersambung): aktif LOW, pull-up.
//   keduanya LOW                -> modul mendorong LOW saat diam = TTP223 standar: aktif HIGH, pull-down.
//   keduanya HIGH               -> modul mendorong HIGH saat diam = TTP223 pad A disolder: aktif LOW.
static void touchInit() {
#if MOCHI_TOUCH_MODE == 1
  touchActiveHigh = true; touchDriven = false;
  const char *kind = "dipaksa aktif HIGH";
#elif MOCHI_TOUCH_MODE == 2
  touchActiveHigh = false; touchDriven = false;
  const char *kind = "dipaksa aktif LOW";
#else
  bool up = sampleHigh(INPUT_PULLUP), down = sampleHigh(INPUT_PULLDOWN);
  const char *kind;
  if (up && !down) { touchActiveHigh = false; touchDriven = false; kind = "tombol ke GND / mengambang, aktif LOW"; }
  else if (!up && !down) { touchActiveHigh = true; kind = "TTP223 standar, aktif HIGH"; }
  else if (up && down) { touchActiveHigh = false; kind = "TTP223 pad A, aktif LOW"; }
  else { touchActiveHigh = true; kind = "tidak jelas, aktif HIGH"; }
#endif
  touchPinMode();
  delay(2);
  lastRaw = touchRawRead();
  debounceAt = millis();
  touchBootAt = millis();
  touchReadyAt = touchBootAt + TOUCH_BOOT_IGNORE_MS;
  waitRelease = lastRaw;
  Serial.printf("sentuh GPIO%d: %s\n", MOCHI_PIN_TOUCH, kind);
}

static Ev readTouch() {
  int raw = touchRawRead();
  uint32_t now = millis();
  // Debounce terhadap bacaan mentah sebelumnya, bukan status stabil.
  if (raw != lastRaw) { lastRaw = raw; debounceAt = now; }
  if (raw && touchDriven && now - touchBootAt < TOUCH_FLIP_WINDOW_MS && now - debounceAt >= TOUCH_STUCK_MS) {
    // TTP223 "tersentuh" terus 10 dtk dalam 60 dtk pertama: salah deteksi saat nyala. Jari menempel belakangan tidak membalik.
    bool wasHold = pressed && holdFired && !longFired;
    touchActiveHigh = !touchActiveHigh;
    touchPinMode();
    delay(2);
    pressed = holdFired = longFired = waitRelease = false;
    taps = 0;
    lastRaw = touchRawRead();
    debounceAt = now;
    Serial.printf("sentuh: aktif terus 10 dtk saat nyala, polaritas dibalik ke aktif %s\n", touchActiveHigh ? "HIGH" : "LOW");
    return wasHold ? Ev::HoldEnd : Ev::None;
  }
  if ((int32_t)(now - touchReadyAt) < 0) return Ev::None;
  if (waitRelease) {
    if (!raw && now - debounceAt >= DEBOUNCE_MS) waitRelease = false;
    return Ev::None;
  }
  if (now - debounceAt >= DEBOUNCE_MS && (raw == 1) != pressed) {
    pressed = raw == 1;
    if (pressed) {
      pressAt = now; holdFired = longFired = false;
    } else {
      if (holdFired) { holdFired = false; if (!longFired) return Ev::HoldEnd; longFired = false; return Ev::None; }
      if (!multiTap) { taps = 0; return Ev::Tap1; }
      taps++; lastTapAt = now;
      if (taps >= 2) { taps = 0; return Ev::Tap2; }
    }
  }
  if (pressed) {
    if (!holdFired && now - pressAt >= HOLD_MS) { holdFired = true; taps = 0; return Ev::HoldStart; }
    if (holdFired && !longFired && now - pressAt >= LONG_MS) { longFired = true; return Ev::Long; }
  } else if (taps && now - lastTapAt >= TAP_GAP_MS) {
    taps = 0; return Ev::Tap1;
  }
  return Ev::None;
}

// =====================================================================
// Animasi JPEG
// =====================================================================
enum class Mode { Stopped, Playing };
static Mode mode = Mode::Stopped;
static int model = 0, clip = 0, frame = 0, savedFrame = 0;
static bool shakeClip = false, holdClip = false;
static uint32_t lastFrame = 0;
static bool mpuOk = false;
static float lastMag = 1.0f;
static int shakeCount = 0;
static uint32_t shakeWindow = 0, lastShake = 0;
static int drawnClip = -1;
static const uint8_t *drawnJpg = nullptr;

static int themeStart(int i) {
  if (i < 0) i = 0;
  if (i >= JPEG_CLIP_COUNT) i = JPEG_CLIP_COUNT - 1;
  const char *theme = JPEG_CLIPS[i].theme;
  int s = i;
  while (s > 0 && strcmp(JPEG_CLIPS[s - 1].theme, theme) == 0) s--;
  return s;
}
static int themeEnd(int i) {
  int s = themeStart(i), e = s;
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

static void forceRedraw() { drawnClip = -1; drawnJpg = nullptr; uiDirty = true; }

static bool drawFrame(int c, int f) {
  if (c < 0 || c >= JPEG_CLIP_COUNT) return false;
  const JpegClip &ci = JPEG_CLIPS[c];
  if (f < 0 || f >= ci.n) return false;
  if (c != drawnClip) {
    if (ci.x || ci.y) tft.fillScreen(TFT_BLACK);  // klip lebih kecil dari layar digambar di tengah
    drawnClip = c;
    drawnJpg = nullptr;
  }
  if (ci.frames[f] == drawnJpg) return true;      // frame sama: tidak perlu decode ulang
  drawnJpg = ci.frames[f];
  return TJpgDec.drawJpg(ci.x, ci.y, ci.frames[f], ci.sizes[f]) == JDR_OK;
}

// Suara animasi hanya jika tidak ada musik, panggilan, atau notifikasi yang sedang berbunyi.
// Chronos UI (overlay notifikasi/panggilan/navigasi/cari/alarm). Butuh drawFrame() di atas.
#include "chronos_ui.inc"

static bool animSoundAllowed() {
  return !musicSession() && dfOwner != Owner::Notif && dfOwner != Owner::Ring && !overlayActive();
}
static void playAudio(int c) {
  if (!animSoundAllowed()) return;
  if (mochiDfPlayTrack(JPEG_CLIPS[c].track)) dfOwner = Owner::Anim;
}

static void stopAll() {
  if (dfOwner == Owner::Anim) { mochiDfStop(); dfOwner = Owner::None; }  // musik tetap jalan
  tft.fillScreen(TFT_BLACK);
  forceRedraw();
  backlight(false);
  mode = Mode::Stopped;
  shakeClip = holdClip = false;
}
static void startMain() {
  if (model < 0 || model >= JPEG_CLIP_COUNT) model = 0;
  clip = mainClip();
  frame = savedFrame = 0;
  shakeClip = holdClip = false;
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
static void holdBegin() {
  if (mode != Mode::Playing || shakeClip) return;
  savedFrame = frame;
  clip = heartClip();
  frame = 0;
  holdClip = true;
  lastFrame = millis();
  playAudio(clip);
}
static void holdEnd() {
  if (!holdClip) return;
  holdClip = false;
  clip = mainClip();
  frame = savedFrame;
  playAudio(clip);
}

static void mpuInit() {
  Wire.begin(MOCHI_PIN_MPU_SDA, MOCHI_PIN_MPU_SCL);
  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0) { Serial.println("MPU6050 tidak ada"); return; }
  const uint8_t init[][2] = {{0x6B, 0x00}, {0x1C, 0x00}, {0x1A, 0x03}};
  for (auto &r : init) { Wire.beginTransmission(0x68); Wire.write(r[0]); Wire.write(r[1]); Wire.endTransmission(); }
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
  float x = ax / 16384.0f, y = ay / 16384.0f, z = az / 16384.0f;
  float mag = sqrtf(x * x + y * y + z * z);
  float delta = fabsf(mag - lastMag);
  lastMag = mag;
  if (now - lastShake < SHAKE_COOLDOWN_MS) return false;
  if (delta > SHAKE_G) {
    if (shakeCount == 0) shakeWindow = now;
    shakeCount++;
    if (shakeCount >= 3 && now - shakeWindow <= SHAKE_WINDOW_MS) { shakeCount = 0; lastShake = now; return true; }
  }
  if (shakeCount && now - shakeWindow > SHAKE_WINDOW_MS) shakeCount = 0;
  return false;
}

static void serviceAnim() {
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
  if (++frame >= JPEG_CLIPS[clip].n) {
    if (shakeClip) {               // klip goyang sekali, lalu kembali ke klip utama
      shakeClip = false;
      clip = mainClip();
      frame = savedFrame;
      playAudio(clip);
    } else {
      frame = 0;                   // gambar mengulang; suara TIDAK diulang di sini, diputar sampai habis
    }
  }
}

// =====================================================================
// Pemutar MP3 (folder /01)
// =====================================================================
static void musicPlay(int n) {
  if (n < 1) n = 1;
  if (n > 255) n = 1;
  mTrack = n;
  if (mochiDfPlayMusic(mTrack)) { dfOwner = Owner::Music; mState = MState::Playing; mEmpty = false; }
  mLastCmd = millis();
  prefs.putUShort("mtrack", mTrack);
  uiDirty = true;
  Serial.printf("musik /01/%03d.mp3\n", mTrack);
}
static void ensureMusicCount() {
  if (mCount >= 0) return;
  int n = mochiDfMusicCount(true);
  if (n > 0) { mCount = n; mEmpty = false; }
  else if (n == 0) mEmpty = true;
}
static int musicNextIndex() {
  ensureMusicCount();
  if (mMode == 2 && mCount > 1) {
    int n;
    do n = 1 + (int)random(mCount); while (n == mTrack);
    return n;
  }
  int n = mTrack + 1;
  if (mCount > 0 && n > mCount) n = 1;
  return n;
}
static void musicNext() { musicPlay(musicNextIndex()); }
static void musicPrev() {
  ensureMusicCount();
  int n = mTrack - 1;
  if (n < 1) n = mCount > 0 ? mCount : 1;
  musicPlay(n);
}
static void musicToggle() {
  if (mState == MState::Playing) {
    mochiDfPause(); mState = MState::Paused;
  } else if (mState == MState::Paused && dfOwner == Owner::Music) {
    mochiDfResume(); mState = MState::Playing;
  } else {
    musicPlay(mTrack);
  }
  mLastCmd = millis();
  uiDirty = true;
}
static void musicStop() {
  if (dfOwner == Owner::Music) { mochiDfStop(); dfOwner = Owner::None; }
  mState = MState::Stopped;
  uiDirty = true;
  if (ui == Ui::Anim && mode == Mode::Playing) playAudio(clip);  // suara animasi kembali
}
static void musicFinished() {
  if (mMode == 1) musicPlay(mTrack);
  else musicNext();
}
static void setVolume(int v) {
  volume = constrain(v, 0, 30);
  mochiDfVolume(volume);
  prefs.putUChar("vol", volume);
  uiDirty = true;
}

// Setelah notifikasi/panggilan selesai: lanjutkan musik (trek diulang dari awal) atau suara animasi.
static void resumeAfterInterrupt() {
  dfOwner = Owner::None;
  if (mState == MState::Playing) musicPlay(mTrack);
  else if (mState == MState::Paused) mState = MState::Stopped;  // posisi jeda hilang setelah diselingi
  else if (ui == Ui::Anim && mode == Mode::Playing) playAudio(clip);
}
// Hook dari chronos_ui.inc. Suara Chronos = trek urutan salin 0041-0048 di root kartu.
// loop: panggilan/cari/alarm diulang sampai overlay ditutup. vol >= 0: volume sementara (cari = 30).
static bool tempVol = false;
void overlaySound(int track, bool loop, int vol) {
  if (dfOwner == Owner::Ring && !loop) return;   // notifikasi tidak memotong dering
  if (vol >= 0 && vol != volume) { mochiDfVolume(vol); tempVol = true; }
  else if (tempVol) { mochiDfVolume(volume); tempVol = false; }
  if (loop) {
    if (mochiDfLoopTrack(track)) dfOwner = Owner::Ring;
  } else if (mochiDfPlayTrack(track)) {
    dfOwner = Owner::Notif;
    notifSoundAt = millis();
  }
}
void overlaySoundEnd() {
  if (dfOwner == Owner::Ring) { mochiDfStop(); dfOwner = Owner::None; }
  if (tempVol) { mochiDfVolume(volume); tempVol = false; }
}
void overlayClosed() {
  if (dfOwner != Owner::Music && dfOwner != Owner::Notif) resumeAfterInterrupt();
  forceRedraw();
  clockDrawn = -1;
  if (ui == Ui::Anim && mode == Mode::Stopped) { tft.fillScreen(TFT_BLACK); backlight(false); }
}

static void serviceSound() {
  mochiDfService();
  uint16_t err;
  if (mochiDfTakeError(&err)) {
    Serial.printf("DFPlayer error %u\n", err);
    bool missing = err == MOCHI_DF_ERR_FILE_MISSING || err == MOCHI_DF_ERR_FILE_INDEX;
    if (dfOwner == Owner::Music && missing) {
      if (mTrack > 1) { mCount = mTrack - 1; musicPlay(1); }  // lewat file terakhir: kembali ke 001
      else { mState = MState::Stopped; dfOwner = Owner::None; mEmpty = true; uiDirty = true; }
    } else if (dfOwner == Owner::Notif && missing) {
      resumeAfterInterrupt();                                  // trek root notifikasi (0041) tidak ada
    }
  }
  if (mochiDfTakeFinished()) {
    switch (dfOwner) {
      case Owner::Music: if (mState == MState::Playing) musicFinished(); break;
      case Owner::Anim:  // trek animasi selesai: ulangi trek klip yang sedang tampil (bukan tiap putaran gambar)
        if (ui == Ui::Anim && mode == Mode::Playing) playAudio(clip); else dfOwner = Owner::None;
        break;
      case Owner::Notif: resumeAfterInterrupt(); break;
      default: break;
    }
  }
  // Pesan selesai tidak datang (mis. TX modul tidak tersambung): anggap notifikasi selesai setelah 8 dtk.
  if (dfOwner == Owner::Notif && millis() - notifSoundAt > 8000) resumeAfterInterrupt();
  // Jangan menanya status untuk ganti lagu: DFPlayer sering menjawab "berhenti" saat masih memutar.
  // Lagu berikutnya hanya dari pesan selesai (0x3D / BUSY) atau error file tidak ada.
}

// =====================================================================
// Gambar: menu, pemutar, jam, info
// =====================================================================
static void serviceBackground() {
  if (chronosOn) {
    watch.loop();
    if (watch.isRunning()) chronoConn = watch.isConnected();
  }
  serviceSound();
}
static void waitMs(uint32_t ms) {
  uint32_t t = millis();
  while (millis() - t < ms) { serviceBackground(); delay(10); }
}
static void showInfo(const char *a, const char *b) {
  backlight(true);
  tft.fillScreen(C_BG);
  tft.fillRoundRect(16, 70, 208, 100, 14, C_SEL);
  tft.setTextColor(TFT_BLACK, C_SEL);
  tft.drawCentreString(a, 120, 90, 2);
  tft.drawCentreString(b, 120, 118, 2);
  waitMs(800);
  forceRedraw();
}

struct MenuItem { const char *label; uint16_t col; };
static const MenuItem MENU[] = {
  {"Pemutar MP3", C_PINK}, {"Pilih nomor", C_PINK}, {"Kembali ke animasi", C_TEAL}, {"Volume +", C_TEAL}, {"Volume -", C_TEAL},
  {"Chronos BLE", C_BLUE}, {"Jam HP", C_BLUE}, {"Tampil navigasi", C_BLUE}, {"Putar layar", C_YEL}, {"Tentang", C_TEXT},
};
static const int NMENU = sizeof(MENU) / sizeof(MENU[0]), VIS = 7;
static int menuRow = 0, menuTop = 0;

static String menuValue(int id) {
  switch (id) {
    case 0: case 1: return mState == MState::Playing ? "main" : (mState == MState::Paused ? "jeda" : "");
    case 3: case 4: return String(volume);
    case 5: return chronosOn ? (chronoConn ? "ON *" : "ON") : "OFF";
    case 6: return clockOn ? "ON" : "OFF";
    case 7: return chronosNav ? "ON" : "OFF";
    case 8: return String(rot * 90) + "°";
    default: return "";
  }
}
static void drawMenu() {
  if (menuRow < menuTop) menuTop = menuRow;
  if (menuRow >= menuTop + VIS) menuTop = menuRow - VIS + 1;
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 240, 34, C_BAR);
  tft.setTextColor(TFT_BLACK, C_BAR);
  tft.drawCentreString("MENU", 120, 4, 2);
  tft.drawCentreString("Mochi DFPlayer " MOCHI_VERSION, 120, 22, 1);
  for (int i = 0; i < VIS; i++) {
    int id = menuTop + i;
    if (id >= NMENU) break;
    int y = 40 + i * 26;
    uint16_t bg = id == menuRow ? C_SEL : C_BG;
    if (id == menuRow) tft.fillRoundRect(6, y - 2, 228, 25, 6, C_SEL);
    tft.fillCircle(20, y + 10, id == menuRow ? 7 : 6, MENU[id].col);
    tft.setTextColor(id == menuRow ? TFT_BLACK : C_TEXT, bg);
    tft.drawString(MENU[id].label, 34, y + 4, 2);
    String v = menuValue(id);
    if (v.length()) {
      tft.setTextColor(id == menuRow ? TFT_BLACK : C_DIM, bg);
      tft.drawRightString(v, 226, y + 4, 2);
    }
  }
  tft.setTextColor(C_DIM, C_BG);
  char foot[64];
  snprintf(foot, sizeof(foot), "%d/%d  ketuk=pindah 2x=pilih tahan 2s=tutup", menuRow + 1, NMENU);
  tft.drawString(foot, 6, 226, 1);
}

// Tombol pemutar: ketuk = jalankan tombol yang disorot, tahan = sorot tombol berikutnya, tahan 2 dtk = menu.
// Pilih nomor hanya menyentuh /01 (bukan trek animasi di root). Musik tetap jalan saat kembali ke animasi.
enum : uint8_t { B_PLAY, B_NEXT, B_PREV, B_PICK, B_VUP, B_VDN, B_MODE, B_STOP, B_COUNT };
static const char *const BTN_NAME[B_COUNT] = {"Putar / jeda", "Trek berikutnya", "Trek sebelumnya", "Pilih nomor",
                                               "Volume +", "Volume -", "Mode putar", "Berhenti"};
static uint8_t pFocus = B_PLAY;
static bool picking = false;
static uint8_t pickDigit = 0;
static uint8_t pickDigs[3] = {0, 0, 1};

static void glyph(uint8_t b, int cx, int cy, uint16_t c) {
  switch (b) {
    case B_PLAY:
      if (mState == MState::Playing) { tft.fillRect(cx - 6, cy - 7, 4, 14, c); tft.fillRect(cx + 2, cy - 7, 4, 14, c); }
      else tft.fillTriangle(cx - 5, cy - 7, cx - 5, cy + 7, cx + 7, cy, c);
      break;
    case B_NEXT: tft.fillTriangle(cx - 7, cy - 7, cx - 7, cy + 7, cx + 3, cy, c); tft.fillRect(cx + 4, cy - 7, 3, 14, c); break;
    case B_PREV: tft.fillTriangle(cx + 7, cy - 7, cx + 7, cy + 7, cx - 3, cy, c); tft.fillRect(cx - 7, cy - 7, 3, 14, c); break;
    case B_VUP: case B_VDN:
      tft.fillRect(cx - 8, cy - 3, 4, 6, c);
      tft.fillTriangle(cx - 5, cy, cx + 1, cy - 7, cx + 1, cy + 7, c);
      tft.fillRect(cx + 3, cy - 1, 6, 2, c);
      if (b == B_VUP) tft.fillRect(cx + 5, cy - 3, 2, 6, c);
      break;
    case B_MODE:
      if (mMode == 2) {
        tft.drawLine(cx - 7, cy - 5, cx + 7, cy + 5, c); tft.drawLine(cx - 7, cy - 4, cx + 7, cy + 6, c);
        tft.drawLine(cx - 7, cy + 5, cx + 7, cy - 5, c); tft.drawLine(cx - 7, cy + 6, cx + 7, cy - 4, c);
      } else {
        tft.drawCircle(cx, cy, 7, c); tft.drawCircle(cx, cy, 6, c);
        tft.fillTriangle(cx + 4, cy - 9, cx + 10, cy - 6, cx + 4, cy - 2, c);
        if (mMode == 1) tft.fillRect(cx - 1, cy - 3, 2, 7, c);
      }
      break;
    case B_STOP: tft.fillRect(cx - 6, cy - 6, 12, 12, c); break;
    case B_PICK:
      tft.drawRoundRect(cx - 7, cy - 8, 14, 16, 2, c);
      tft.drawFastHLine(cx - 4, cy - 2, 8, c);
      tft.drawFastHLine(cx - 4, cy + 2, 8, c);
      break;
  }
}
static void startPick() {
  ensureMusicCount();
  int n = mTrack;
  if (n < 1) n = 1;
  if (n > 255) n = 255;
  pickDigs[0] = n / 100;
  pickDigs[1] = (n / 10) % 10;
  pickDigs[2] = n % 10;
  pickDigit = 2;
  picking = true;
  uiDirty = true;
}
static int pickValue() { return pickDigs[0] * 100 + pickDigs[1] * 10 + pickDigs[2]; }
static void confirmPick() {
  int n = pickValue();
  if (n < 1) n = 1;
  if (mCount > 0 && n > mCount) n = mCount;
  if (n > 255) n = 255;
  picking = false;
  musicPlay(n);  // /01 saja; tidak menyentuh trek animasi di root
}
static void drawEq() {
  static uint8_t ph = 0;
  ph++;
  tft.fillRect(186, 48, 44, 44, C_BG);
  for (int i = 0; i < 5; i++) {
    int h = mState == MState::Playing ? 8 + (int)((sinf((ph + i * 2) * 0.9f) + 1.0f) * 16) : 6;
    tft.fillRoundRect(188 + i * 8, 92 - h, 6, h, 2, mState == MState::Playing ? C_PINK : C_DIM);
  }
}
static void drawPlayer() {
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 240, 34, C_BAR);
  tft.setTextColor(TFT_BLACK, C_BAR);
  tft.drawCentreString("PEMUTAR MP3", 120, 4, 2);
  tft.drawCentreString("kartu DFPlayer, folder /01", 120, 22, 1);
  char b[24];
  if (picking) snprintf(b, sizeof(b), "%d%d%d", pickDigs[0], pickDigs[1], pickDigs[2]);
  else snprintf(b, sizeof(b), "%03d", mTrack);
  tft.setTextColor(picking ? C_YEL : C_TEXT, C_BG);
  tft.setTextSize(2);
  tft.drawString(b, 14, 42, 4);
  tft.setTextSize(1);
  if (picking) {
    int dx = 14 + pickDigit * 24;
    tft.drawFastHLine(dx, 90, 20, C_YEL);
  }
  tft.setTextColor(C_DIM, C_BG);
  if (mEmpty) snprintf(b, sizeof(b), "folder /01 kosong");
  else if (mCount > 0) snprintf(b, sizeof(b), "dari %d lagu", mCount);
  else snprintf(b, sizeof(b), "jumlah lagu ?");
  tft.drawString(b, 16, 98, 2);
  drawEq();
  const char *st = mState == MState::Playing ? "Memutar" : (mState == MState::Paused ? "Jeda" : "Berhenti");
  uint16_t sc = mState == MState::Playing ? C_TEAL : (mState == MState::Paused ? C_YEL : C_DIM);
  tft.fillCircle(20, 126, 5, sc);
  tft.setTextColor(sc, C_BG);
  tft.drawString(st, 30, 119, 2);
  int mw = tft.textWidth(MODE_NAME[mMode], 2) + 16;
  tft.fillRoundRect(226 - mw, 117, mw, 20, 10, C_BLUE);
  tft.setTextColor(TFT_WHITE, C_BLUE);
  tft.drawCentreString(MODE_NAME[mMode], 226 - mw / 2, 119, 2);
  tft.setTextColor(C_TEXT, C_BG);
  snprintf(b, sizeof(b), "Vol %d", volume);
  tft.drawString(b, 14, 145, 2);
  for (int i = 0; i < 15; i++)
    tft.fillRoundRect(76 + i * 10, 147, 8, 12, 2, i < (volume + 1) / 2 ? C_TEAL : 0x2945);
  tft.setTextColor(C_SEL, C_BG);
  tft.drawCentreString(BTN_NAME[pFocus], 120, 168, 2);
  for (int i = 0; i < B_COUNT; i++) {
    int x = 4 + i * 29, y = 188;
    bool f = i == pFocus && !picking;
    if (f) tft.fillRoundRect(x, y, 27, 30, 8, C_SEL);
    else { tft.fillRoundRect(x, y, 27, 30, 8, C_BG); tft.drawRoundRect(x, y, 27, 30, 8, C_DIM); }
    glyph(i, x + 13, y + 15, f ? TFT_BLACK : C_TEXT);
  }
  tft.setTextColor(C_DIM, C_BG);
  tft.drawCentreString(picking ? "ketuk=angka  tahan=digit  2 dtk=batal" : "ketuk=jalankan  tahan=pindah  2 dtk=menu", 120, 226, 1);
}

static const char *const WD[7] = {"MIN", "SEN", "SEL", "RAB", "KAM", "JUM", "SAB"};
static void eye(int cx, int cy, int open) {
  tft.fillRect(cx - 28, cy - 20, 56, 42, TFT_BLACK);
  tft.fillRoundRect(cx - 24, cy - 8, 48, 20, 8, TFT_WHITE);
  if (open <= 0) {
    tft.fillRoundRect(cx - 24, cy - 8, 48, 16, 6, TFT_BLACK);
    tft.drawWideLine(cx - 22, cy + 2, cx + 22, cy + 2, 2, TFT_WHITE, TFT_BLACK);
    return;
  }
  int lid = 18 - open * 3;
  if (lid < 0) lid = 0;
  tft.fillCircle(cx, cy + 2, 7, 0x4A69);
  tft.fillCircle(cx, cy + 2, 3, TFT_BLACK);
  tft.fillCircle(cx - 2, cy, 1, TFT_WHITE);
  if (lid > 0) tft.fillRoundRect(cx - 24, cy - 10, 48, lid, 4, TFT_BLACK);
}
static void blob(int x, int y, int w, int h) { tft.fillRoundRect(x, y, w, h, h / 2, TFT_WHITE); }
static void ring(int x, int y, int w, int h, int t) {
  tft.fillRoundRect(x, y, w, h, h / 3, TFT_WHITE);
  tft.fillRoundRect(x + t, y + t, w - 2 * t, h - 2 * t, (h - 2 * t) / 3, TFT_BLACK);
}
static void digitR(int x, int y, int d) {
  int w = 36, h = 46, t = 8;
  tft.fillRect(x, y, w, h, TFT_BLACK);
  if (d == 0) ring(x, y, w, h, t);
  else if (d == 1) blob(x + w - t - 2, y, t, h);
  else if (d == 2) { ring(x, y, w, h / 2 + 2, t); blob(x, y + h - t, w, t); blob(x, y + h / 2 - 2, t, h / 2); }
  else if (d == 3) { ring(x, y, w, h / 2 + 2, t); ring(x, y + h / 2 - 2, w, h / 2 + 2, t); }
  else if (d == 4) { blob(x, y, t, h / 2 + 2); blob(x, y + h / 2 - t / 2, w, t); blob(x + w - t, y, t, h); }
  else if (d == 5) { blob(x, y, w, t); blob(x, y, t, h / 2); ring(x, y + h / 2 - 2, w, h / 2 + 2, t); }
  else if (d == 6) { blob(x, y, t, h); ring(x, y + h / 2 - 2, w, h / 2 + 2, t); blob(x, y, w, t); }
  else if (d == 7) { blob(x, y, w, t); blob(x + w - t, y, t, h); }
  else if (d == 8) { ring(x, y, w, h / 2 + 2, t); ring(x, y + h / 2 - 2, w, h / 2 + 2, t); }
  else { ring(x, y, w, h / 2 + 2, t); blob(x + w - t, y, t, h); blob(x, y + h - t, w, t); }
}
static void drawClock(bool full) {
  bool linked = chronosOn && watch.isRunning() && watch.isConnected();
  int h = linked ? watch.getHour(true) : 0, m = linked ? watch.getMinute() : 0, s = linked ? watch.getSecond() : 0;
  int sig = linked ? (h * 3600 + m * 60 + s) : -2;
  if (full || sig != clockDrawn) {
  if (full) tft.fillScreen(TFT_BLACK);
  if (sig / 60 != clockDrawn / 60) clockInfoDirty = true;   // baterai HP tiap menit
  clockDrawn = sig;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (linked) {
    int wd = watch.getDayofWeek();
    if (wd < 0 || wd > 6) wd = 0;
    char top[8];
    snprintf(top, sizeof(top), "%02d", watch.getDay());
    tft.drawCentreString(top, 70, 8, 4);
    tft.drawCentreString(WD[wd], 170, 14, 2);
    int x = 18, y = 36;
    digitR(x, y, h / 10); x += 42;
    digitR(x, y, h % 10); x += 38;
    uint16_t col = (s & 1) ? TFT_WHITE : TFT_BLACK;
    tft.fillCircle(x + 4, y + 14, 3, col);
    tft.fillCircle(x + 4, y + 30, 3, col);
    x += 14;
    digitR(x, y, m / 10); x += 42;
    digitR(x, y, m % 10);
  } else {
    tft.drawCentreString("--", 70, 8, 4);
    tft.drawCentreString("---", 170, 14, 2);
    tft.drawCentreString("--:--", 120, 48, 4);
  }
  int blink = (s % 5 == 0) ? 0 : (s % 5 == 1) ? 2 : 6;
  eye(78, 132, blink);
  eye(162, 132, blink);
  }
  bool msg = clockMsgUntil && (int32_t)(millis() - clockMsgUntil) < 0;
  if (!msg && clockMsgUntil) { clockMsgUntil = 0; clockInfoDirty = true; }
  if (!full && !clockInfoDirty) return;
  clockInfoDirty = false;
  tft.fillRect(0, 160, 240, 80, TFT_BLACK);
  if (linked) {
    // baris 1: baterai HP + cuaca, baris 2: musik HP
    char nb[48];
    if (weather.ok) {
      int ic = weather.icon >= 0 && weather.icon < 8 ? weather.icon : 0;
      snprintf(nb, sizeof(nb), "HP %d%%  %dC %s", watch.getPhoneBattery(), weather.temp, CUACA[ic]);
    } else {
      snprintf(nb, sizeof(nb), "HP %d%%", watch.getPhoneBattery());
    }
    tft.drawCentreString(fitText(nb, 2, 232), 120, 166, 2);
    if (weather.ok && weather.city.length()) {
      tft.setTextColor(C_DIM, TFT_BLACK);
      snprintf(nb, sizeof(nb), "%d/%dC", weather.high, weather.low);
      tft.drawCentreString(fitText(toAscii(weather.city) + "  " + nb, 1, 232), 120, 184, 1);
    }
    tft.setTextColor(msg ? C_SEL : C_TEAL, TFT_BLACK);
    String mu = msg ? clockMsg
                    : music.ok ? String(music.playing ? "> " : "|| ") + toAscii(music.title) +
                                     (music.artist.length() ? " - " + toAscii(music.artist) : String(""))
                               : String("musik HP: ketuk = putar/jeda");
    tft.drawCentreString(fitText(mu, 2, 232), 120, 198, 2);
  } else {
    tft.drawCentreString(chronosOn ? "sambungkan Chronos" : "Chronos BLE mati", 120, 182, 2);
  }
  tft.setTextColor(0x4A69, TFT_BLACK);
  tft.drawCentreString(linked ? "ketuk putar/jeda  2x lagu  tahan 2s menu" : "tahan 2 dtk = menu", 120, 226, 1);
}
static void showAbout() {
  backlight(true);
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 240, 34, C_BAR);
  tft.setTextColor(TFT_BLACK, C_BAR);
  tft.drawCentreString("TENTANG", 120, 9, 2);
  tft.setTextColor(C_TEXT, C_BG);
  char b[48];
  int y = 46;
  auto line = [&](const char *t) { tft.drawString(t, 12, y, 2); y += 22; };
  snprintf(b, sizeof(b), "Mochi DFPlayer %s", MOCHI_VERSION); line(b);
  snprintf(b, sizeof(b), "%d model animasi", JPEG_CLIP_COUNT); line(b);
  snprintf(b, sizeof(b), "BLE: %s", MOCHI_BLE_NAME); line(b);
  snprintf(b, sizeof(b), "Chronos: %s", chronosOn ? (chronoConn ? "tersambung" : "menunggu HP") : "mati"); line(b);
  if (chronoConn) { snprintf(b, sizeof(b), "Baterai HP: %d%%", watch.getPhoneBattery()); line(b); }
  snprintf(b, sizeof(b), "Trek animasi: %s",
#ifdef MOCHI_DF_MP3_FOLDER
           "/MP3/000N.mp3"
#else
           "urutan salin"
#endif
  ); line(b);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawCentreString("MIT (c) rzmong", 120, 222, 1);
  waitMs(3000);
  forceRedraw();
}

// =====================================================================
// Pindah layar + aksi menu
// =====================================================================
static bool openPicker = false;
static void enterUi(Ui u) {
  if (ui == Ui::Anim && u != Ui::Anim) holdEnd();
  ui = u;
  multiTap = u != Ui::Player;
  backlight(true);
  forceRedraw();
  if (u == Ui::Anim) {
    if (mode != Mode::Playing) startMain();
    else if (dfOwner != Owner::Anim) playAudio(clip);
  } else if (dfOwner == Owner::Anim) {
    mochiDfStop(); dfOwner = Owner::None;   // suara animasi tidak ikut ke menu/jam/pemutar
  }
  if (u == Ui::Player && mCount < 0) {
    int n = mochiDfMusicCount(true);
    if (n > 0) { mCount = n; mEmpty = false; }
    else if (n == 0) mEmpty = true;
    if (mCount > 0 && mTrack > mCount) mTrack = 1;
  }
  if (u == Ui::Player && openPicker) { openPicker = false; startPick(); }
  if (u == Ui::Clock) clockDrawn = -1;
}
static void openMenu() { menuRow = 0; menuTop = 0; enterUi(Ui::Menu); }

static void applyMenu() {
  switch (menuRow) {
    case 0: pFocus = B_PLAY; picking = false; enterUi(Ui::Player); return;
    case 1: openPicker = true; enterUi(Ui::Player); return;
    case 2: clockOn = false; savePrefs(); enterUi(Ui::Anim); return;
    case 3: setVolume(volume + 2); break;
    case 4: setVolume(volume - 2); break;
    case 5: chronosOn = !chronosOn; if (!chronosOn) clockOn = false; savePrefs(); chronosApply();
            showInfo("Chronos BLE", chronosOn ? "ON: buka aplikasi Chronos" : "OFF"); break;
    case 6: clockOn = !clockOn;
            if (clockOn && !chronosOn) { chronosOn = true; chronosApply(); }
            savePrefs();
            if (clockOn) { enterUi(Ui::Clock); return; }
            break;
    case 7: chronosNav = !chronosNav; savePrefs(); break;
    case 8: rot = (rot + 1) & 3; tft.setRotation(rot); savePrefs(); break;
    case 9: showAbout(); break;
  }
  uiDirty = true;
}

static void playerAction(uint8_t b) {
  switch (b) {
    case B_PLAY: musicToggle(); break;
    case B_NEXT: musicNext(); break;
    case B_PREV: musicPrev(); break;
    case B_PICK: startPick(); return;
    case B_VUP: setVolume(volume + 2); break;
    case B_VDN: setVolume(volume - 2); break;
    case B_MODE: mMode = (mMode + 1) % 3; prefs.putUChar("mmode", mMode); break;
    case B_STOP: musicStop(); break;
  }
  uiDirty = true;
}
static void playerPickTap() {
  pickDigs[pickDigit] = (pickDigs[pickDigit] + 1) % 10;
  uiDirty = true;
}
static void playerPickHold() {
  if (pickDigit < 2) { pickDigit++; uiDirty = true; return; }
  confirmPick();
}

// =====================================================================
// Overlay Chronos (menutup layar apa pun)
// =====================================================================
static bool serviceOverlay(Ev ev) {
  if (!serviceChronosOverlay()) return false;
  backlight(true);
  if (ui == Ui::Anim && holdClip) holdEnd();
  // Ketuk / tahan = tutup overlay teratas (panggilan: hanya bisukan; tolak panggilan tidak didukung Chronos).
  if (ev == Ev::Tap1 || ev == Ev::Tap2 || ev == Ev::HoldStart) dismissOverlay();
  return true;
}

// =====================================================================
void setup() {
  pinMode(MOCHI_PIN_TFT_BL, OUTPUT);
  digitalWrite(MOCHI_PIN_TFT_BL, HIGH);
  Serial.begin(115200);
  touchInit();
  prefs.begin("mochidfp", false);
  loadPrefs();
  SPI.begin(MOCHI_PIN_TFT_SCLK, -1, MOCHI_PIN_TFT_MOSI, -1);
  tft.init();
  tft.setRotation(rot);
  tft.fillScreen(TFT_BLACK);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback([](int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) -> bool {
    if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
  });
  mochiDfVolume(volume);
  mochiDfInit();
  mpuInit();
  randomSeed(esp_random());
  chronosSetupCallbacks();
  if (chronosOn) chronosApply();
  overlaySoundEnd();
  model = bootModel();
  Serial.printf("Mochi DFPlayer %s, %d model, rotasi %d, Chronos %s\n", MOCHI_VERSION, JPEG_CLIP_COUNT, rot,
                chronosOn ? "ON" : "OFF");
  ui = homeUi();
  multiTap = true;
  if (ui == Ui::Anim) startMain();
}

void loop() {
  serviceBackground();
  Ev ev = readTouch();
  if (serviceOverlay(ev)) { delay(20); return; }

  switch (ui) {
    case Ui::Anim:
      if (ev == Ev::Tap1) { if (mode == Mode::Playing) stopAll(); else startMain(); }
      else if (ev == Ev::Tap2) nextModel();
      else if (ev == Ev::HoldStart) holdBegin();
      else if (ev == Ev::HoldEnd) holdEnd();
      else if (ev == Ev::Long) { openMenu(); break; }
      if (uiDirty) { uiDirty = false; drawnJpg = nullptr; }
      serviceAnim();
      break;
    case Ui::Clock:
      if (ev == Ev::Long) { openMenu(); break; }
      if ((ev == Ev::Tap1 || ev == Ev::Tap2) && chronoConn) {   // kontrol musik di HP
        watch.musicControl(ev == Ev::Tap1 ? MUSIC_TOGGLE : MUSIC_NEXT);
        clockMessage(ev == Ev::Tap1 ? "musik HP: putar/jeda" : "musik HP: berikutnya");
      }
      drawClock(uiDirty);
      uiDirty = false;
      delay(20);
      break;
    case Ui::Menu:
      if (ev == Ev::Tap1) { menuRow = (menuRow + 1) % NMENU; uiDirty = true; }
      else if (ev == Ev::Tap2) applyMenu();
      else if (ev == Ev::Long) { enterUi(homeUi()); break; }
      if (uiDirty && ui == Ui::Menu) { uiDirty = false; drawMenu(); }
      delay(15);
      break;
    case Ui::Player: {
      if (picking) {
        if (ev == Ev::Tap1) playerPickTap();
        else if (ev == Ev::HoldEnd) playerPickHold();
        else if (ev == Ev::Long) { picking = false; uiDirty = true; }
      } else if (ev == Ev::Tap1) playerAction(pFocus);
      else if (ev == Ev::HoldEnd) { pFocus = (pFocus + 1) % B_COUNT; uiDirty = true; }
      else if (ev == Ev::Long) { openMenu(); break; }
      if (uiDirty) { uiDirty = false; drawPlayer(); }
      static uint32_t eqAt = 0;
      if (millis() - eqAt > 150) { eqAt = millis(); drawEq(); }
      delay(10);
      break;
    }
  }
}
