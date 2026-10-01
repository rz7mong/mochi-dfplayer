#include "MochiDfPlayer.h"
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "MochiRzmong.h"

/* Salinan jalur suara firmware MAX98357.
 * Di sana GPIO20/21 = I2S. Di sini UART DFPlayer. Jangan pasang MAX98357.
 * RX modul <- GPIO20 lewat 1k. TX modul -> GPIO21. VCC = 5V.
 *
 * SD modul (FAT32):
 *   01/001..011  reaksi, urutan MOCHI_REACT
 *   02/001..     ekspresi/wajah, nomor = indeks bawaan + 1
 *   06/001..009  tema, urutan MOCHI_THEMES
 *   03/001       notifikasi Chronos
 *   04/001       dering Chronos (diulang)
 *   05/001..     lagu pemutar MP3
 */
static const int DF_RX = 21;
static const int DF_TX = 20;
static const int DF_FOLDER_REACT = 1;
static const int DF_FOLDER_FACE = 2;
static const int DF_FOLDER_NOTIF = 3;
static const int DF_FOLDER_RING = 4;
static const int DF_FOLDER_MUSIC = 5;
static const int DF_FOLDER_THEME = 6;

static HardwareSerial dfSerial(1);
static DFRobotDFPlayerMini dfPlayer;
static bool dfOk = false;
static bool musicOn = false;
static bool ringOn = false;
static uint32_t dfLastCmd = 0;

static void dfGap() {
  uint32_t now = millis();
  if (now - dfLastCmd < 80) delay(80 - (now - dfLastCmd));
  dfLastCmd = millis();
}

static int dfVol(int vol21) {
  if (vol21 < 0) vol21 = 0;
  if (vol21 > 21) vol21 = 21;
  return (vol21 * 30) / 21;
}

static void dfPlay(int folder, int file) {
  if (!dfOk || file < 1) return;
  ringOn = false;
  if (folder != DF_FOLDER_MUSIC) musicOn = false;
  dfGap();
  dfPlayer.playFolder(folder, file);
}

bool mochiDfInit() {
  dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
  delay(200);
  dfOk = dfPlayer.begin(dfSerial, false, false);
  if (!dfOk) {
    Serial.println("DFPlayer tidak jawab");
    return false;
  }
  dfPlayer.setTimeOut(500);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  dfPlayer.volume(dfVol(12));
  Serial.println("DFPlayer siap");
  return true;
}

void mochiDfStop() {
  musicOn = false;
  ringOn = false;
  if (!dfOk) return;
  dfGap();
  dfPlayer.stop();
}

void mochiDfService() {}

void mochiDfSetVolume(int vol21, bool on) {
  if (!dfOk) return;
  dfGap();
  dfPlayer.volume(on ? dfVol(vol21) : 0);
  if (!on) dfPlayer.pause();
}

bool mochiDfPlayReact(int reactIndex) {
  if (!dfOk) return false;
  if (reactIndex < 0 || reactIndex >= MOCHI_REACT_COUNT) reactIndex = 0;
  dfPlay(DF_FOLDER_REACT, reactIndex + 1);
  return true;
}

bool mochiDfPlayFace(int faceIndex) {
  if (!dfOk) return false;
  if (faceIndex < 0) faceIndex = 0;
  dfPlay(DF_FOLDER_FACE, faceIndex + 1);
  return true;
}

bool mochiDfPlayTheme(const char *theme) {
  if (!dfOk || !theme) return false;
  for (int i = 0; i < MOCHI_THEME_COUNT; i++) {
    if (strcmp(theme, MOCHI_THEMES[i]) == 0) {
      dfPlay(DF_FOLDER_THEME, i + 1);
      return true;
    }
  }
  return false;
}

bool mochiDfPlayGif(const char *gifPath) {
  if (!dfOk || !gifPath) return false;
  const char *slash = strrchr(gifPath, '/');
  const char *base = slash ? slash + 1 : gifPath;
  char stem[48];
  strncpy(stem, base, sizeof(stem) - 1);
  stem[sizeof(stem) - 1] = 0;
  char *dot = strrchr(stem, '.');
  if (dot) *dot = 0;
  for (int i = 0; i < MOCHI_REACT_COUNT; i++) {
    if (strcmp(stem, MOCHI_REACT[i].stem) == 0 || strcmp(stem, MOCHI_REACT[i].name) == 0)
      return mochiDfPlayReact(i);
  }
  const char *gif = strstr(gifPath, "/gif/");
  if (gif) {
    gif += 5;
    char tema[24];
    const char *cut = strchr(gif, '/');
    if (cut && cut - gif < (int)sizeof(tema)) {
      memcpy(tema, gif, cut - gif);
      tema[cut - gif] = 0;
      if (mochiDfPlayTheme(tema)) return true;
    }
  }
  return mochiDfPlayFace(0);
}

bool mochiDfPlayNotif() {
  if (!dfOk) return false;
  dfPlay(DF_FOLDER_NOTIF, 1);
  return true;
}

void mochiDfPlayRinger(bool on) {
  if (!dfOk) return;
  if (!on) {
    if (ringOn) mochiDfStop();
    return;
  }
  musicOn = false;
  ringOn = true;
  dfGap();
  dfPlayer.loopFolder(DF_FOLDER_RING);
}

bool mochiDfMusicStart() {
  if (!dfOk) return false;
  ringOn = false;
  musicOn = true;
  dfGap();
  dfPlayer.loopFolder(DF_FOLDER_MUSIC);
  return true;
}

bool mochiDfMusicNext() {
  if (!dfOk) return false;
  musicOn = true;
  ringOn = false;
  dfGap();
  dfPlayer.next();
  return true;
}

bool mochiDfMusicPrev() {
  if (!dfOk) return false;
  musicOn = true;
  ringOn = false;
  dfGap();
  dfPlayer.previous();
  return true;
}

void mochiDfMusicToggle() {
  if (!dfOk) return;
  if (musicOn) {
    musicOn = false;
    dfGap();
    dfPlayer.pause();
  } else {
    mochiDfMusicStart();
  }
}

void mochiDfMusicStop() { mochiDfStop(); }
bool mochiDfMusicPlaying() { return musicOn; }
