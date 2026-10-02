// chronos_ui.inc — Chronos (ChronosESP32, BLE) untuk Mochi DFPlayer. Di-include oleh src/main.cpp.
// Butuh dari main.cpp (sebelum include): tft, watch, warna C_*, chronosOn, chronosNav, uiDirty,
// dan hook suara overlaySound(trek, ulang, volume) / overlaySoundEnd().
//
// Callback ChronosESP32 berjalan di task BLE (NimBLE). Di sana data hanya disalin (dengan mutex) dan diberi tanda;
// layar dan DFPlayer hanya disentuh dari loop() lewat chronosEvents() dan serviceChronosOverlay().
// Semua suara adalah MP3 di kartu DFPlayer, nomor urut salin 0041-0048 (TRK_* di MochiRzmong.h).
void overlaySound(int track, bool loop, int vol);
void overlaySoundEnd();
void overlayClosed();

static const uint16_t C_FAINT = 0x4208, C_PANEL = 0x18E3, C_DRED = 0x5000, C_GREEN = 0x2E8A, C_NAVY = 0x0866,
                      C_ORANGE = 0xFC40, C_WHITE = 0xFFFF;

bool chronoConn = false;
static bool timeSynced = false;
static SemaphoreHandle_t chLock;
struct NavCopy {
  bool active = false, isNav = false, hasIcon = false;
  String title, directions, distance, duration, eta, speed;
  uint8_t icon[CS_ICON_DATA_SIZE];
  uint32_t crc = 0;
};
static NavCopy navIn, nav;  // navIn: ditulis callback; nav: dipakai loop
static Notification notifIn;
static String callerIn;
static volatile bool evNotif = false, evNav = false, evFind = false, evTime = false, evWeather = false, evMusic = false;
static volatile uint8_t evConn = 0;  // 1 sambung, 2 putus
static volatile uint8_t evRing = 0;  // 1 mulai, 2 selesai
struct WeatherCopy { bool ok = false; int temp = 0, high = 0, low = 0, icon = 0; String city; };
struct MusicCopy { bool ok = false; bool playing = false; String title, artist; };
static WeatherCopy weatherIn, weather;
static MusicCopy musicIn, music;

static void lockCh() { xSemaphoreTake(chLock, portMAX_DELAY); }
static void unlockCh() { xSemaphoreGive(chLock); }

static void onChConn(bool c) { evConn = c ? 1 : 2; }
static void onChNotif(Notification n) {
  lockCh();
  notifIn = n;
  evNotif = true;
  unlockCh();
}
static void onChRinger(String caller, bool on) {
  lockCh();
  if (on) callerIn = caller;
  evRing = on ? 1 : 2;
  unlockCh();
}
static void copyNav() {
  Navigation &n = watch.getNavigation();
  navIn.active = n.active;
  navIn.isNav = n.isNavigation;
  navIn.hasIcon = n.hasIcon;
  navIn.title = n.title;
  navIn.directions = n.directions;
  navIn.distance = n.distance;
  navIn.duration = n.duration;
  navIn.eta = n.eta;
  navIn.speed = n.speed;
  memcpy(navIn.icon, n.icon, sizeof(navIn.icon));
  navIn.crc = n.iconCRC;
}
static void onChConfig(Config c, uint32_t a, uint32_t b) {
  switch (c) {
    case CF_FIND: evFind = true; break;
    case CF_TIME: if (a == 1) evTime = true; break;
    case CF_NAV_DATA:
    case CF_NAV_ICON:
      if (c == CF_NAV_ICON && a != 2) break;  // ikon dikirim bertahap; a == 2 = lengkap
      lockCh();
      copyNav();
      evNav = true;
      unlockCh();
      break;
    case CF_WEATHER:
      if (watch.getWeatherCount() > 0) {
        lockCh();
        Weather &w = watch.getWeatherAt(0);
        weatherIn.ok = true;
        weatherIn.temp = w.temp;
        weatherIn.high = w.high;
        weatherIn.low = w.low;
        weatherIn.icon = w.icon;
        weatherIn.city = watch.getWeatherCity();
        evWeather = true;
        unlockCh();
      }
      break;
    case CF_MUSIC: {
      lockCh();
      MusicInfo &m = watch.getMusicInfo();
      musicIn.ok = m.title.length() > 0;
      musicIn.playing = b == 1;
      musicIn.title = m.title;
      musicIn.artist = m.artist;
      evMusic = true;
      unlockCh();
      break;
    }
    default: break;
  }
}
// ---------- Teks: UTF-8 -> ASCII (font bawaan TFT_eSPI hanya ASCII) ----------
static String toAscii(const String &in) {
  String out;
  out.reserve(in.length());
  const uint8_t *p = (const uint8_t *)in.c_str();
  while (*p) {
    uint32_t cp;
    int n;
    if (*p < 0x80) { cp = *p; n = 1; }
    else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1F; n = 2; }
    else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0F; n = 3; }
    else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07; n = 4; }
    else { p++; continue; }
    int k = 1;
    for (; k < n && (p[k] & 0xC0) == 0x80; k++) cp = (cp << 6) | (p[k] & 0x3F);
    p += k;
    if (k < n) continue;
    char c = 0;
    if (cp == '\n' || cp == '\r' || cp == '\t') c = ' ';
    else if (cp >= 0x20 && cp < 0x7F) c = (char)cp;
    else if (cp == 0x2018 || cp == 0x2019) c = '\'';
    else if (cp == 0x201C || cp == 0x201D) c = '"';
    else if (cp == 0x2013 || cp == 0x2014) c = '-';
    else if (cp == 0x2026) { out += "..."; continue; }
    else if (cp == 0xA0) c = ' ';
    else if (cp >= 0xC0 && cp <= 0xFF) {
      static const char map[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPsaaaaaaaceeeeiiiidnooooo/ouuuuypy";
      c = map[cp - 0xC0];
    }
    if (c && !(c == ' ' && out.length() && out[out.length() - 1] == ' ')) out += c;
  }
  out.trim();
  return out;
}

// Bungkus teks per kata ke lebar maxW (piksel) untuk font f. Kata yang terlalu panjang dipotong paksa.
static int wrapText(const String &s, uint8_t f, int maxW, String *lines, int maxLines) {
  int n = 0;
  String cur;
  int i = 0, len = s.length();
  while (i < len && n < maxLines) {
    int j = s.indexOf(' ', i);
    if (j < 0) j = len;
    String word = s.substring(i, j);
    i = j + 1;
    if (!word.length()) continue;
    String test = cur.length() ? cur + " " + word : word;
    if (tft.textWidth(test, f) <= maxW) { cur = test; continue; }
    if (cur.length()) { lines[n++] = cur; cur = ""; if (n >= maxLines) break; }
    while (tft.textWidth(word, f) > maxW && n < maxLines) {  // kata panjang (URL dsb.)
      int k = word.length();
      while (k > 1 && tft.textWidth(word.substring(0, k), f) > maxW) k--;
      lines[n++] = word.substring(0, k);
      word = word.substring(k);
    }
    cur = word;
  }
  if (cur.length() && n < maxLines) lines[n++] = cur;
  return n;
}
static String fitText(const String &s, uint8_t f, int maxW) {
  if (tft.textWidth(s, f) <= maxW) return s;
  String t = s;
  while (t.length() > 1 && tft.textWidth(t + "...", f) > maxW) t.remove(t.length() - 1);
  return t + "...";
}

// ---------- Overlay ----------
enum Ov : uint8_t { OV_NONE, OV_INFO, OV_NAV, OV_NOTIF, OV_ALARM, OV_CALL, OV_FIND };  // makin besar makin penting
static Ov shownOv = OV_NONE;
static bool ovDirty = false;
static uint32_t infoUntil = 0, notifUntil = 0, alarmUntil = 0, findUntil = 0;
static String infoText;
static bool infoOk = false;
static bool navHidden = false;
static bool callOn = false, callMuted = false;
static String callName;

static bool until(uint32_t t) { return t && (int32_t)(millis() - t) < 0; }
static Ov topOverlay() {
  if (!chronosOn) return OV_NONE;
  if (until(findUntil)) return OV_FIND;
  if (callOn && !callMuted) return OV_CALL;
  if (until(alarmUntil)) return OV_ALARM;
  if (until(notifUntil)) return OV_NOTIF;
  if (chronosNav && nav.active && !navHidden) return OV_NAV;
  if (until(infoUntil)) return OV_INFO;
  return OV_NONE;
}
bool overlayActive() { return topOverlay() != OV_NONE; }
// Suara hanya jika overlay itu yang paling atas (notifikasi tidak memotong dering panggilan, dst.).
static void soundFor(Ov ov, int track, bool loop = false, int vol = -1) {
  if (topOverlay() == ov) overlaySound(track, loop, vol);
}

// Info singkat 1,5 s (sambung/putus, navigasi selesai).
static void chInfo(const char *text, bool ok, int track) {
  infoText = text;
  infoOk = ok;
  infoUntil = millis() + 1500;
  ovDirty = true;
  if (topOverlay() <= OV_NAV) overlaySound(track, false, -1);
}

// Notifikasi
static String nApp, nTitle, nTime;
static String nLines[24];
static int nCount = 0, nScroll = 0;
static uint32_t nScrollAt = 0;
static uint16_t nColor = C_TEAL;
static const int N_VISIBLE = 6;

static uint16_t appColor(const String &app) {
  static const uint16_t pal[] = {0x05F7, 0x2E8A, 0xFC40, 0xA254, 0x3A7F, 0xE8EC, 0x8E60};
  uint32_t h = 7;
  for (size_t i = 0; i < app.length(); i++) h = h * 31 + (uint8_t)app[i];
  String a = app;
  a.toLowerCase();
  if (a.indexOf("whatsapp") >= 0) return 0x25C9;
  if (a.indexOf("telegram") >= 0) return 0x24FC;
  if (a.indexOf("gmail") >= 0 || a.indexOf("youtube") >= 0) return 0xD8A3;
  return pal[h % (sizeof(pal) / sizeof(pal[0]))];
}

static void drawNotifBody() {
  tft.fillRect(0, 102, 240, 116, C_BG);
  tft.setTextColor(0xDEFB, C_BG);
  for (int i = 0; i < N_VISIBLE && nScroll + i < nCount; i++) tft.drawString(nLines[nScroll + i], 10, 104 + i * 19, 2);
  if (nCount > N_VISIBLE) {  // bilah gulir
    int track = 112, top = 104;
    int h = track * N_VISIBLE / nCount;
    int y = top + (track - h) * nScroll / (nCount - N_VISIBLE);
    tft.fillRect(234, top, 3, track, C_FAINT);
    tft.fillRect(234, y, 3, h, C_DIM);
  }
}
static void drawNotif() {
  tft.fillScreen(C_BG);
  tft.fillRoundRect(0, 0, 240, 44, 0, nColor);
  tft.fillCircle(24, 22, 15, C_TEXT);
  tft.setTextColor(nColor, C_TEXT);
  String ini = nApp.length() ? nApp.substring(0, 1) : String("N");
  ini.toUpperCase();
  tft.setTextDatum(MC_DATUM);
  tft.drawString(ini, 24, 23, 4);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, nColor);
  tft.drawString(fitText(nApp.length() ? nApp : String("Notifikasi"), 2, 132), 48, 23, 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(nTime, 232, 23, 2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  if (tft.textWidth(nTitle, 4) <= 220) tft.drawString(nTitle, 10, 58, 4);
  else {
    String t[2];
    int n = wrapText(nTitle, 2, 220, t, 2);
    for (int i = 0; i < n; i++) tft.drawString(i == 1 ? fitText(t[1], 2, 220) : t[i], 10, 56 + i * 18, 2);
  }
  tft.drawFastHLine(10, 96, 220, C_FAINT);
  drawNotifBody();
  tft.setTextColor(C_FAINT, C_BG);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("ketuk = tutup", 120, 238, 1);
  tft.setTextDatum(TL_DATUM);
}
// Notifikasi dibuka dengan klip wajah/cinta_pipi (sekali, ±1,5 s) sebelum teksnya tampil.
static int introClip = -2;
static uint32_t introAt = 0, introLen = 0;
static bool introOn = false;
static int findIntroClip() {
  for (int i = 0; i < JPEG_CLIP_COUNT; i++)
    if (strcmp(JPEG_CLIPS[i].stem, "cinta_pipi") == 0) return i;
  return -1;
}
static void showNotif(const Notification &n) {
  if (introClip == -2) introClip = findIntroClip();
  introOn = introClip >= 0;
  introAt = millis();
  introLen = introOn ? (uint32_t)JPEG_CLIPS[introClip].n * JPEG_CLIPS[introClip].delay : 0;
  nApp = toAscii(n.app);
  nTitle = toAscii(n.title);
  nTime = toAscii(n.time);
  if (nTime.length() > 5) nTime = nTime.substring(0, 5);
  nColor = appColor(nApp);
  String msg = toAscii(n.message);
  if (nTitle.length() == 0) nTitle = nApp.length() ? nApp : String("Notifikasi");
  nCount = wrapText(msg, 2, 220, nLines, 24);
  nScroll = 0;
  nScrollAt = millis() + introLen + 3000;
  uint32_t dur = 7000 + (nCount > N_VISIBLE ? (nCount - N_VISIBLE) * 1500 + 2000 : 0);
  notifUntil = millis() + introLen + (dur > 30000 ? 30000 : dur);
  ovDirty = true;
  if (!watch.isQuietActive()) soundFor(OV_NOTIF, TRK_NOTIF);
  Serial.printf("notif %s: %s (%d baris)\n", nApp.c_str(), nTitle.c_str(), nCount);
}
static bool drawFrame(int c, int f);
static void serviceNotif() {
  if (introOn) {
    uint32_t t = millis() - introAt;
    if (t < introLen) {
      drawFrame(introClip, t / JPEG_CLIPS[introClip].delay);
      return;
    }
    introOn = false;
    drawNotif();
  }
  if (nCount > N_VISIBLE && nScroll < nCount - N_VISIBLE && (int32_t)(millis() - nScrollAt) >= 0) {
    nScroll++;
    nScrollAt = millis() + 1500;
    drawNotifBody();
  }
}

// Navigasi
static String navKey;
static uint32_t navSoundAt = 0;
static void drawNavIcon(int x0, int y0, uint16_t fg, uint16_t bg) {
  static uint16_t row[96];
  for (int y = 0; y < 48; y++) {
    for (int x = 0; x < 48; x++) {
      int bi = (y * 48 + x) / 8;
      bool on = (nav.icon[bi] >> (7 - (x % 8))) & 1;
      uint16_t c = on ? fg : bg;
      c = (c >> 8) | (c << 8);  // pushImage dengan swap bytes aktif
      row[2 * x] = row[2 * x + 1] = c;
    }
    tft.pushImage(x0, y0 + 2 * y, 96, 1, row);
    tft.pushImage(x0, y0 + 2 * y + 1, 96, 1, row);
  }
}
static void drawNav() {
  tft.fillScreen(C_NAVY);
  String t = toAscii(nav.title), d = toAscii(nav.directions);
  String lines[4];
  int textX = 10, textW = 220, yDir = 70;
  if (nav.hasIcon) {
    tft.fillRoundRect(6, 6, 108, 108, 12, C_PANEL);
    drawNavIcon(12, 12, C_TEXT, C_PANEL);
    int n = wrapText(t, 4, 112, lines, 3);
    tft.setTextColor(C_YEL, C_NAVY);
    for (int i = 0; i < n; i++) tft.drawString(lines[i], 122, 14 + i * 28, 4);
    yDir = 122;
  } else {
    int n = wrapText(t, 4, textW, lines, 2);
    tft.setTextColor(C_YEL, C_NAVY);
    for (int i = 0; i < n; i++) tft.drawString(lines[i], textX, 14 + i * 28, 4);
  }
  int n = wrapText(d, 2, textW, lines, nav.hasIcon ? 4 : 6);
  tft.setTextColor(C_TEXT, C_NAVY);
  for (int i = 0; i < n; i++) tft.drawString(lines[i], textX, yDir + i * 19, 2);
  tft.fillRect(0, 202, 240, 38, C_PANEL);
  String foot;
  String eta = toAscii(nav.eta), dist = toAscii(nav.distance), dur = toAscii(nav.duration), spd = toAscii(nav.speed);
  if (eta.length()) foot += "Tiba " + eta;
  if (dist.length()) foot += (foot.length() ? "  " : "") + dist;
  if (dur.length()) foot += (foot.length() ? "  " : "") + dur;
  if (spd.length()) foot += (foot.length() ? "  " : "") + spd;
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(fitText(foot.length() ? foot : String("Navigasi"), 2, 228), 120, 221, 2);
  tft.setTextDatum(TL_DATUM);
}
static void applyNav() {
  bool wasActive = nav.active;
  lockCh();
  nav = navIn;
  unlockCh();
  String key = nav.title + "|" + nav.directions;
  if (nav.active) {
    if (!wasActive || key != navKey) {  // instruksi baru: tampilkan lagi walau tadi disembunyikan
      navHidden = false;
      if (millis() - navSoundAt > 2000) {
        navSoundAt = millis();
        soundFor(OV_NAV, TRK_NAV);
      }
    }
    navKey = key;
    ovDirty = true;
  } else if (wasActive) {
    navKey = "";
    navHidden = false;
    chInfo("Navigasi selesai", true, TRK_NAV_END);
  }
}

// Panggilan
static void drawCall(bool pulse) {
  if (!pulse) {
    tft.fillScreen(C_DRED);
    tft.setTextColor(C_TEXT, C_DRED);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("Panggilan masuk", 120, 14, 2);
    String lines[2];
    int n = wrapText(callName.length() ? callName : String("Tidak dikenal"), 4, 224, lines, 2);
    for (int i = 0; i < n; i++) tft.drawString(lines[i], 120, 150 + i * 28, 4);
    tft.setTextColor(0xFBEF, C_DRED);
    tft.drawString("ketuk = senyapkan", 120, 222, 1);
    tft.setTextDatum(TL_DATUM);
  }
  static bool big = false;
  big = !big;
  tft.fillCircle(120, 84, 52, C_DRED);
  tft.drawCircle(120, 84, big ? 50 : 46, C_GREEN);
  tft.fillCircle(120, 84, 40, C_GREEN);
  // gagang telepon sederhana
  tft.fillRoundRect(100, 66, 14, 22, 5, C_TEXT);
  tft.fillRoundRect(126, 80, 14, 22, 5, C_TEXT);
  tft.drawLine(108, 86, 130, 84, C_TEXT);
  for (int k = -3; k <= 3; k++) tft.drawLine(106, 80 + k, 128, 98 + k, C_TEXT);
}

// Cari perangkat
static void drawFind(bool on) {
  uint16_t bg = on ? C_YEL : C_BG, fg = on ? C_BG : C_YEL;
  tft.fillScreen(bg);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("DI SINI!", 120, 100, 4);
  tft.setTextSize(1);
  tft.drawString("Mochi dicari dari HP", 120, 160, 2);
  tft.drawString("ketuk untuk berhenti", 120, 200, 2);
  tft.setTextDatum(TL_DATUM);
}

// Alarm
static uint32_t alarmKey = 0xFFFFFFFF;
static void drawAlarm(bool on) {
  uint16_t bg = on ? C_ORANGE : C_BG, fg = on ? C_BG : C_ORANGE;
  tft.fillScreen(bg);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ALARM", 120, 50, 4);
  char b[8];
  snprintf(b, sizeof(b), "%02d:%02d", watch.getHour(true), watch.getMinute());
  tft.drawString(b, 120, 120, 7);
  tft.drawString("ketuk = matikan", 120, 200, 2);
  tft.setTextDatum(TL_DATUM);
}
static void checkAlarms() {
  static uint32_t lastCheck = 0;
  if (!timeSynced || millis() - lastCheck < 1000) return;
  lastCheck = millis();
  uint32_t key = (uint32_t)watch.getDayofYear() * 1440 + watch.getHour(true) * 60 + watch.getMinute();
  if (key == alarmKey) return;
  for (int i = 0; i < CS_ALARM_SIZE; i++) {
    if (watch.isAlarmActive(i)) {
      alarmKey = key;
      alarmUntil = millis() + 60000;
      ovDirty = true;
      soundFor(OV_ALARM, TRK_ALARM, true);
      Serial.printf("alarm %d\n", i);
      return;
    }
  }
}

// Info sambung/putus
static void drawInfo() {
  tft.fillScreen(C_BG);
  uint16_t c = infoOk ? C_TEAL : C_DIM;
  tft.fillRoundRect(20, 70, 200, 100, 16, C_PANEL);
  tft.fillCircle(120, 100, 14, c);
  tft.setTextColor(C_BG, c);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(infoOk ? "i" : "!", 120, 101, 2);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.drawString(infoText, 120, 140, 2);
  tft.setTextDatum(TL_DATUM);
}


// ---------- Halaman jam: cuaca dan musik HP (dipakai drawClock di main.cpp) ----------
static const char *const CUACA[] = {"Cerah berawan", "Cerah", "Salju", "Hujan", "Berawan", "Badai", "Berangin", "Berkabut"};
static bool clockInfoDirty = true;
static String clockMsg;
static uint32_t clockMsgUntil = 0;
static void clockMessage(const char *msg) {
  clockMsg = msg;
  clockMsgUntil = millis() + 2000;
  clockInfoDirty = true;
}

// ---------- Layar overlay ----------
static uint32_t ovAnimAt = 0;
static bool ovPhase = false;
static void drawOverlay(Ov ov) {
  ovPhase = false;
  ovAnimAt = millis();
  switch (ov) {
    case OV_INFO: drawInfo(); break;
    case OV_NAV: drawNav(); break;
    case OV_NOTIF: if (introOn) tft.fillScreen(TFT_BLACK); else drawNotif(); break;
    case OV_ALARM: drawAlarm(true); break;
    case OV_CALL: drawCall(false); break;
    case OV_FIND: drawFind(true); break;
    default: break;
  }
}
// Satu sentuhan menutup overlay teratas.
static void dismissOverlay() {
  switch (topOverlay()) {
    case OV_INFO: infoUntil = 0; break;
    case OV_NAV: navHidden = true; break;
    case OV_NOTIF: notifUntil = 0; introOn = false; break;
    case OV_ALARM: alarmUntil = 0; break;
    case OV_CALL: callMuted = true; break;
    case OV_FIND: findUntil = 0; break;
    default: break;
  }
}

static void chronosEvents() {
  if (evConn) {
    bool c = evConn == 1;
    evConn = 0;
    chronoConn = c;
    if (!c) {
      callOn = false;
      nav.active = false;  // navigasi berhenti diam-diam saat terputus
      navKey = "";
      navHidden = false;
    } else {
      watch.setBattery(100, false);  // perangkat tanpa sensor baterai
    }
    chInfo(c ? "Chronos tersambung" : "Chronos terputus", c, c ? TRK_CONNECT : TRK_DISCONN);
    clockInfoDirty = true;
    uiDirty = true;
    Serial.printf("[chronos] %s\n", c ? "tersambung" : "terputus");
  }
  if (evTime) {
    evTime = false;
    if (!timeSynced) Serial.printf("[chronos] waktu %s\n", watch.getTimeDate().c_str());
    timeSynced = true;
    uiDirty = true;
  }
  if (evNotif) {
    evNotif = false;
    lockCh();
    Notification n = notifIn;
    unlockCh();
    showNotif(n);
  }
  if (evRing) {
    uint8_t r = evRing;
    evRing = 0;
    if (r == 1) {
      lockCh();
      callName = toAscii(callerIn);
      unlockCh();
      callOn = true;
      callMuted = false;
      ovDirty = true;
      soundFor(OV_CALL, TRK_CALL, true);
    } else {
      callOn = false;
    }
  }
  if (evFind) {
    evFind = false;
    findUntil = millis() + 60000;
    ovDirty = true;
    soundFor(OV_FIND, TRK_FIND, true, 30);
  }
  if (evNav) {
    evNav = false;
    applyNav();
  }
  if (evWeather) {
    evWeather = false;
    lockCh();
    weather = weatherIn;
    unlockCh();
    clockInfoDirty = true;
  }
  if (evMusic) {
    evMusic = false;
    lockCh();
    music = musicIn;
    unlockCh();
    clockInfoDirty = true;
  }
  checkAlarms();
}

// Tiap loop: ganti layar overlay bila perlu dan jalankan animasinya. true = overlay sedang menutupi layar.
static bool serviceChronosOverlay() {
  if (!chronosOn) return false;
  chronosEvents();
  Ov top = topOverlay();
  if (top != shownOv) {
    Ov old = shownOv;
    shownOv = top;
    if (top == OV_NONE) {
      overlaySoundEnd();   // hentikan dering/alarm/cari yang diulang
      overlayClosed();     // main.cpp: gambar ulang layar dan lanjutkan suara animasi/musik
      return false;
    }
    if (old == OV_CALL || old == OV_FIND || old == OV_ALARM) overlaySoundEnd();
    drawOverlay(top);
    ovDirty = false;
  } else if (top != OV_NONE && ovDirty) {
    ovDirty = false;
    drawOverlay(top);
  }
  if (top == OV_NONE) return false;
  if (top == OV_NOTIF) serviceNotif();
  if ((top == OV_FIND || top == OV_ALARM || top == OV_CALL) && millis() - ovAnimAt >= (top == OV_CALL ? 600u : 400u)) {
    ovAnimAt = millis();
    ovPhase = !ovPhase;
    if (top == OV_FIND) drawFind(!ovPhase);
    else if (top == OV_ALARM) drawAlarm(!ovPhase);
    else drawCall(true);
  }
  return true;
}

void chronosSetupCallbacks() {
  if (!chLock) chLock = xSemaphoreCreateMutex();
  watch.setConnectionCallback(onChConn);
  watch.setNotificationCallback(onChNotif);
  watch.setRingerCallback(onChRinger);
  watch.setConfigurationCallback(onChConfig);
}
void chronosApply() {
  if (chronosOn) {
    if (!watch.isRunning()) {
      chronosSetupCallbacks();
      watch.set24Hour(true);
      watch.begin();
      watch.setBattery(100, false);
      Serial.printf("[chronos] BLE nyala, nama \"%s\" mac %s\n", MOCHI_BLE_NAME, watch.getAddress().c_str());
    }
  } else {
    chronoConn = false;
    notifUntil = alarmUntil = findUntil = infoUntil = 0;
    callOn = false;
    nav.active = false;
    navHidden = false;
    if (watch.isRunning()) watch.stop();
    Serial.println("[chronos] BLE mati");
  }
}
