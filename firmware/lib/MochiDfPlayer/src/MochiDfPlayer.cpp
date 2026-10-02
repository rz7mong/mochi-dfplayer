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
 *
 * Pemutar JPEG (src/main.cpp) memakai mochiDfPlayTrack(n):
 *   default            -> perintah 0x03 play(n): file ke-n menurut URUTAN SALIN di FAT root,
 *                         bukan nama file. Salin 0001.mp3, 0002.mp3, ... satu per satu, berurutan.
 *   -DMOCHI_DF_MP3_FOLDER -> perintah 0x12 playMp3Folder(n): /MP3/0001.mp3 dst., dicocokkan dari NAMA file.
 */
static const int DF_RX = MOCHI_PIN_DF_RX;
static const int DF_TX = MOCHI_PIN_DF_TX;
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
static int musicFile = 1;
static uint32_t dfLastCmd = 0;
// Volume 0..30 yang diminta (dfVol(20) = 28, sama seperti volume tetap 28 sebelumnya).
static int dfVolWanted = 28;
static int dfVolSent = -1;
// DFPlayer butuh waktu sesudah daya masuk untuk membaca kartu SD; perintah sebelum itu diabaikan modul.
static const uint32_t DF_BOOT_MS = 1200;

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
  // Tunggu modul selesai boot (dihitung dari nyala ESP), supaya perintah awal tidak hilang.
  if (millis() < DF_BOOT_MS) delay(DF_BOOT_MS - millis());
  // isACK=false: begin() selalu true walau modul tidak tersambung (tidak ada jawaban yang dicek).
  dfOk = dfPlayer.begin(dfSerial, false, false);
  if (!dfOk) {
    Serial.println("DFPlayer tidak jawab");
    return false;
  }
  dfPlayer.setTimeOut(500);
  dfGap();
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfGap();
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  dfGap();
  dfPlayer.volume(dfVolWanted);
  dfVolSent = dfVolWanted;
  Serial.println("DFPlayer siap (UART dikirim, tanpa ACK)");
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
  if (on) dfVolWanted = dfVol(vol21);
  if (!dfOk) return;
  int v = on ? dfVolWanted : 0;
  dfGap();
  dfPlayer.volume(v);
  dfVolSent = v;
  if (!on) {
    dfGap();
    dfPlayer.pause();
  }
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
  if (musicFile < 1) musicFile = 1;
  dfPlay(DF_FOLDER_MUSIC, musicFile);
  musicOn = true;
  return true;
}

bool mochiDfMusicNext() {
  if (!dfOk) return false;
  musicFile++;
  return mochiDfMusicStart();
}

bool mochiDfMusicPrev() {
  if (!dfOk) return false;
  if (musicFile > 1) musicFile--;
  return mochiDfMusicStart();
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

bool mochiDfPlayTrack(uint8_t track) {
  if (!dfOk || track == 0) {
    mochiDfStop();
    return false;
  }
  musicOn = false;
  ringOn = false;
  // Dulu volume(28) dikirim tiap trek tepat sebelum play() tanpa jeda; modul bisa menelan perintah kedua.
  // Sekarang volume hanya dikirim jika berubah, dan selalu ada jeda antarperintah.
  if (dfVolSent != dfVolWanted) {
    dfGap();
    dfPlayer.volume(dfVolWanted);
    dfVolSent = dfVolWanted;
  }
  dfGap();
#ifdef MOCHI_DF_MP3_FOLDER
  dfPlayer.playMp3Folder(track);
#else
  dfPlayer.play(track);
#endif
  return true;
}
