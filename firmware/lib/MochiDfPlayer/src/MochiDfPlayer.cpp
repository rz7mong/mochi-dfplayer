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
static bool dfPresent = false;   // modul menjawab query status (hasil mochiDfInit)
static bool dfCardOut = false;   // pesan 0x3B: kartu SD dicabut
static bool dfReinit = false;    // sedang menunggu / mencoba menyiapkan ulang modul setelah kartu masuk
static uint8_t dfReinitTry = 0;
static uint32_t dfReinitAt = 0;
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
static const uint32_t DF_CARD_MOUNT_MS = 1000;  // jeda setelah kartu masuk: modul butuh waktu membaca kartu
static const uint32_t DF_COUNT_BUDGET_MS = 450; // batas total mochiDfMusicCount (tiap percobaan s/d ±280 ms)

// Perintah hanya dikirim bila modul aktif, kartu terpasang, dan tidak sedang disiapkan ulang.
static bool dfUsable() { return dfOk && !dfCardOut && !dfReinit; }

static void dfGap() {
  uint32_t now = millis();
  if (now - dfLastCmd < 80) delay(80 - (now - dfLastCmd));
  dfLastCmd = millis();
}

static void noteFinished() {
  uint32_t now = millis();
  // Pesan 0x3D sering dobel, dan sisa trek sebelumnya bisa datang tepat setelah perintah putar baru.
  // Jendela setelah play sengaja pendek: nada Chronos (sambung/putus) bisa selesai dalam ~200 ms.
  if (now - lastFinishAt < 400 || now - dfLastPlay < 120) return;
  lastFinishAt = now;
  finished = true;
}

static void handleMsg(uint8_t type, uint16_t value) {
  if (type == DFPlayerPlayFinished) noteFinished();
  else if (type == DFPlayerError && value != TimeOut) { errPending = true; errCode = value; }
  else if (type == DFPlayerCardRemoved) {
    Serial.println("DFPlayer: kartu SD dicabut");
    dfCardOut = true;
    dfReinit = false;
    finished = false;
    busyWasPlaying = false;
    loopTrack = 0;
    musicCount = -2;  // jumlah lagu bisa berubah bila kartu diganti
  } else if (type == DFPlayerCardInserted || (type == DFPlayerCardOnline && dfCardOut)) {
    Serial.println("DFPlayer: kartu SD dipasang, menyiapkan ulang");
    dfCardOut = false;
    dfReinit = true;
    dfReinitTry = 0;
    dfReinitAt = millis() + DF_CARD_MOUNT_MS;
    musicCount = -2;
  }
}

// Baca semua pesan yang sudah masuk (tiap pesan diproses). Dipanggil sebelum query agar pesan lama
// (atau flag TimeOut sisa) tidak dikira jawaban query. Selalu berhenti: available() mengosongkan flag.
static void dfDrain() {
  for (uint8_t i = 0; i < 8 && dfPlayer.available(); i++) handleMsg(dfPlayer.readType(), dfPlayer.read());
}

// Dipanggil hanya setelah query GAGAL: library sudah menerima pesan lain (mis. trek selesai, kartu dicabut)
// atau mengisi TimeOut. Karena dfDrain() dijalankan sebelum query, isinya pasti baru. TimeOut diabaikan handleMsg.
static void dfTakeLastReply() { handleMsg(dfPlayer.readType(), dfPlayer.read()); }

// Query status (0x42) dengan antrian bersih. Memblok maks. dfGap() + setTimeOut (±280 ms). -1 = tak menjawab.
static int dfReadState() {
  dfDrain();
  dfGap();
  int s = dfPlayer.readState();
  if (s < 0) dfTakeLastReply();
  return s;
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
  int st = dfReadState();
  if (st < 0) st = dfReadState();  // modul kadang baru menjawab pada percobaan kedua
  dfPresent = st >= 0;
  Serial.printf("DFPlayer %s (status %d)\n", dfPresent ? "menjawab" : "tidak menjawab, perintah tetap dikirim", st);
  // dfOk tetap true walau tidak menjawab: TX modul bisa saja tidak tersambung sementara suara tetap jalan.
  // Hasil nyata dikembalikan ke pemanggil dan tersedia lewat mochiDfPresent().
  return dfPresent;
}

bool mochiDfReady() { return dfUsable(); }
bool mochiDfPresent() { return dfPresent; }
bool mochiDfCardOut() { return dfCardOut; }

// Siapkan ulang modul setelah kartu dipasang. Tanpa blokir panjang: tiap langkah dijadwalkan ulang
// dengan backoff (1 dtk, 2 dtk); percobaan ke-3 memakai reset 0x0C lalu menunggu modul menyala kembali.
static void serviceReinit() {
  if (!dfReinit || (int32_t)(millis() - dfReinitAt) < 0) return;
  if (dfReinitTry == 2) {
    Serial.println("DFPlayer: reset modul (0x0C)");
    dfGap();
    dfPlayer.reset();
    dfVolSent = -1;
    dfReinitTry++;
    dfReinitAt = millis() + 2500;  // modul butuh ±1,5 dtk untuk menyala dan membaca kartu
    return;
  }
  dfGap();
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  dfVolSent = -1;
  dfGap();
  dfPlayer.volume(dfVolWanted);
  dfVolSent = dfVolWanted;
  int st = dfReadState();
  if (st >= 0) {
    dfPresent = true;
    dfReinit = false;
    Serial.println("DFPlayer: siap kembali");
  } else if (++dfReinitTry > 3) {
    dfPresent = false;
    dfReinit = false;  // menyerah; perintah dikirim lagi seperti pada init yang tidak dijawab
    Serial.println("DFPlayer: tidak menjawab setelah kartu dipasang");
  } else {
    dfReinitAt = millis() + (500u << dfReinitTry);
  }
}

void mochiDfService() {
  if (!dfOk) return;
  while (dfSerial.available() && dfPlayer.available()) handleMsg(dfPlayer.readType(), dfPlayer.read());
  serviceReinit();
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
  if (dfUsable()) sendVolumeIfNeeded();
}
int mochiDfGetVolume() { return dfVolWanted; }

void mochiDfStop() {
  if (!dfUsable()) return;
  dfGap();
  dfPlayer.stop();
  loopTrack = 0;
  finished = false;
  busyWasPlaying = false;
}
void mochiDfPause() {
  if (!dfUsable()) return;
  dfGap();
  dfPlayer.pause();
  busyWasPlaying = false;  // BUSY naik saat jeda; jangan dianggap selesai
}
void mochiDfResume() {
  if (!dfUsable()) return;
  dfGap();
  dfPlayer.start();
  markPlay();
}

bool mochiDfPlayTrack(uint16_t track) {
  if (!dfUsable() || track == 0) return false;
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
  if (!dfUsable() || track == 0) return false;
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
  if (!dfUsable() || file < 1 || file > 255) return false;
  sendVolumeIfNeeded();
  dfGap();
  dfPlayer.playFolder(MOCHI_DF_FOLDER_MUSIC, file);
  markPlay();
  loopTrack = 0;
  return true;
}

int mochiDfMusicCount(bool refresh) {
  if (!dfUsable()) return -1;
  if (musicCount >= 0 && !refresh) return musicCount;
  int n = -1;
  // Query folder sering tidak dijawab pada percobaan pertama. Kegagalan tidak di-cache.
  // Total waktu dibatasi (DF_COUNT_BUDGET_MS) agar UI tidak macet ±1 dtk bila modul diam.
  uint32_t t0 = millis();
  for (int i = 0; i < 4 && n < 0 && millis() - t0 < DF_COUNT_BUDGET_MS; i++) {
    dfDrain();
    dfGap();
    n = dfPlayer.readFileCountsInFolder(MOCHI_DF_FOLDER_MUSIC);
    if (n < 0) dfTakeLastReply();
  }
  if (n >= 0) musicCount = n;
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
  if (!dfUsable()) return DfState::Unknown;
  int s = dfReadState();  // bila yang datang pesan lain (mis. trek selesai), tetap diproses, tidak hilang
  if (s < 0) return DfState::Unknown;
  switch (s & 0x0F) {
    case 1: return DfState::Playing;
    case 2: return DfState::Paused;
    default: return DfState::Stopped;
  }
}

bool mochiDfBusyPin() { return MOCHI_PIN_DF_BUSY >= 0; }
