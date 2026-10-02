#include "MochiDfPlayer.h"
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "MochiRzmong.h"

/* Kabel: RX modul <- GPIO20 lewat 1k. TX modul -> GPIO21. VCC = 5V. BUSY -> MOCHI_PIN_DF_BUSY (opsional).
 * Selesai-trek dideteksi dari pesan UART 0x3D (modul mengirimnya sendiri, sering dua kali) dan, jika ada,
 * dari pin BUSY yang naik ke HIGH. Status juga bisa ditanya (0x42) lewat mochiDfQueryState(). */
static HardwareSerial dfSerial(1);
static DFRobotDFPlayerMini dfPlayer;
static bool dfOk = false;
static uint32_t dfLastCmd = 0;
static uint32_t dfLastPlay = 0;
static int dfVolWanted = 28;
static int dfVolSent = -1;
static bool finished = false;
static uint32_t lastFinishAt = 0;
static bool errPending = false;
static uint16_t errCode = 0;
static bool busyWasPlaying = false;
static int musicCount = -2;
static uint16_t loopTrack = 0;  // != 0: trek yang sedang diulang  // -2 = belum ditanya
static const uint32_t DF_BOOT_MS = 1200;

static void dfGap() {
  uint32_t now = millis();
  if (now - dfLastCmd < 80) delay(80 - (now - dfLastCmd));
  dfLastCmd = millis();
}

static void noteFinished() {
  uint32_t now = millis();
  // Abaikan pesan ganda dan pesan sisa trek sebelumnya yang datang tepat setelah perintah putar baru.
  if (now - lastFinishAt < 400 || now - dfLastPlay < 300) return;
  lastFinishAt = now;
  finished = true;
}

static void handleMsg(uint8_t type, uint16_t value) {
  if (type == DFPlayerPlayFinished) noteFinished();
  else if (type == DFPlayerError && value != TimeOut) { errPending = true; errCode = value; }
}

static void sendVolumeIfNeeded() {
  if (dfVolSent == dfVolWanted) return;
  dfGap();
  dfPlayer.volume(dfVolWanted);
  dfVolSent = dfVolWanted;
}

static void markPlay() {
  dfLastPlay = millis();
  finished = false;
  busyWasPlaying = false;
}

bool mochiDfInit() {
#if MOCHI_PIN_DF_BUSY >= 0
  pinMode(MOCHI_PIN_DF_BUSY, INPUT_PULLUP);
#endif
  dfSerial.begin(9600, SERIAL_8N1, MOCHI_PIN_DF_RX, MOCHI_PIN_DF_TX);
  delay(200);
  if (millis() < DF_BOOT_MS) delay(DF_BOOT_MS - millis());  // modul butuh waktu membaca kartu
  // isACK=false: begin() selalu true walau modul tidak tersambung. Kehadiran modul dicek lewat query di bawah.
  dfOk = dfPlayer.begin(dfSerial, false, false);
  dfPlayer.setTimeOut(200);
  dfGap();
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfGap();
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  sendVolumeIfNeeded();
  dfGap();
  int st = dfPlayer.readState();
  Serial.printf("DFPlayer %s (status %d)\n", st >= 0 ? "menjawab" : "tidak menjawab, perintah tetap dikirim", st);
  return dfOk;
}

bool mochiDfReady() { return dfOk; }

void mochiDfService() {
  if (!dfOk) return;
  while (dfSerial.available() && dfPlayer.available()) handleMsg(dfPlayer.readType(), dfPlayer.read());
#if MOCHI_PIN_DF_BUSY >= 0
  bool playing = digitalRead(MOCHI_PIN_DF_BUSY) == LOW;
  if (playing && millis() - dfLastPlay > 150) busyWasPlaying = true;
  if (!playing && busyWasPlaying && millis() - dfLastPlay > 600) {
    busyWasPlaying = false;
    noteFinished();
  }
#endif
}

void mochiDfVolume(int v) {
  if (v < 0) v = 0;
  if (v > 30) v = 30;
  dfVolWanted = v;
  if (dfOk) sendVolumeIfNeeded();
}
int mochiDfGetVolume() { return dfVolWanted; }

void mochiDfStop() {
  if (!dfOk) return;
  dfGap();
  dfPlayer.stop();
  loopTrack = 0;
  finished = false;
  busyWasPlaying = false;
}
void mochiDfPause() {
  if (!dfOk) return;
  dfGap();
  dfPlayer.pause();
  busyWasPlaying = false;  // BUSY naik saat jeda; jangan dianggap selesai
}
void mochiDfResume() {
  if (!dfOk) return;
  dfGap();
  dfPlayer.start();
  markPlay();
}

bool mochiDfPlayTrack(uint16_t track) {
  if (!dfOk || track == 0) return false;
  sendVolumeIfNeeded();
  dfGap();
#ifdef MOCHI_DF_MP3_FOLDER
  dfPlayer.playMp3Folder(track);
#else
  dfPlayer.play(track);  // bawaan: urutan salin di root (0x03)
#endif
  markPlay();
  loopTrack = 0;
  return true;
}

// Trek diulang terus sampai mochiDfStop() / trek lain (dering panggilan, cari perangkat, alarm).
bool mochiDfLoopTrack(uint16_t track) {
  if (!dfOk || track == 0) return false;
  sendVolumeIfNeeded();
  dfGap();
#ifdef MOCHI_DF_MP3_FOLDER
  dfPlayer.playMp3Folder(track);  // /MP3 tidak punya perintah ulang: diputar lagi saat selesai
#else
  dfPlayer.loop(track);           // 0x08: ulang satu trek (urutan salin)
#endif
  markPlay();
  loopTrack = track;
  return true;
}

bool mochiDfPlayMusic(uint16_t file) {
  if (!dfOk || file < 1 || file > 255) return false;
  sendVolumeIfNeeded();
  dfGap();
  dfPlayer.playFolder(MOCHI_DF_FOLDER_MUSIC, file);
  markPlay();
  loopTrack = 0;
  return true;
}

int mochiDfMusicCount(bool refresh) {
  if (!dfOk) return -1;
  if (musicCount != -2 && !refresh) return musicCount;
  int n = -1;
  for (int i = 0; i < 2 && n < 0; i++) {
    dfGap();
    n = dfPlayer.readFileCountsInFolder(MOCHI_DF_FOLDER_MUSIC);
    if (n < 0) handleMsg(dfPlayer.readType(), dfPlayer.read());
  }
  musicCount = n;
  return n;
}

bool mochiDfTakeFinished() {
  if (!finished) return false;
  finished = false;
  if (loopTrack) {
#ifdef MOCHI_DF_MP3_FOLDER
    uint16_t t = loopTrack;
    mochiDfLoopTrack(t);
#endif
    return false;  // trek ulang tidak dilaporkan selesai
  }
  return true;
}

bool mochiDfTakeError(uint16_t *code) {
  if (!errPending) return false;
  errPending = false;
  if (code) *code = errCode;
  return true;
}

DfState mochiDfQueryState() {
  if (!dfOk) return DfState::Unknown;
  dfGap();
  int s = dfPlayer.readState();
  if (s < 0) {  // bisa jadi yang datang pesan lain (mis. trek selesai): jangan sampai hilang
    handleMsg(dfPlayer.readType(), dfPlayer.read());
    return DfState::Unknown;
  }
  switch (s & 0x0F) {
    case 1: return DfState::Playing;
    case 2: return DfState::Paused;
    default: return DfState::Stopped;
  }
}

bool mochiDfBusyPin() { return MOCHI_PIN_DF_BUSY >= 0; }
