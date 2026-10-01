// Mochi rzmong DFPlayer — salinan firmware MAX98357, suara lewat library MochiDfPlayer (MP3 di SD modul)
// Nama AP dan sandi tetap MOCHI_AP_NAME / MOCHI_AP_PASS. Aset GIF tidak diubah.
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <ChronosESP32.h>
#include <MochiDfPlayer.h>
#include "MochiRzmong.h"
#include "defaults_gif.h"
#include "jingle.h"
#include "captive_ui.h"

static const int W=240,H=240;
static const uint16_t C_BG=0x1082,C_BAR=0xFD20,C_SEL=0xFE60,C_TEXT=0xEF7D,C_DIM=0x8410;
static const uint16_t C_RED=0xF985,C_TEAL=0x07F4,C_BLUE=0x3C7F,C_PINK=0xF81F,C_YEL=0xFFE0;

TFT_eSPI tft; AnimatedGIF gif; WebServer server(80); DNSServer dnsServer; ChronosESP32 watch("rzmong", CF_ESP32_240x240); Preferences prefs;
String theme="wajah", playMode="kategori", reactMode="acak", apPass=MOCHI_AP_PASS;
bool soundOn=true, useSd=false, chronosOn=false, showWm=true, clockOn=false;
bool chronoConn=false, ringerOn=false, chronosNav=true;
String notifApp, notifTitle, notifMsg, ringerName;
uint32_t notifUntil=0;
int defIdx=0, reactIdx=0, volume=12, rot=MOCHI_DEFAULT_ROTATION, menuRow=0, menuTop=0;
String parts[32]; int nparts=0, idx=0; File gifFile; bool sdOk=false, i2sOk=false, sdBusy=false;
enum Ui { UI_PLAY, UI_MENU }; Ui ui=UI_PLAY;
uint32_t downAt=0,lastTap=0; int taps=0; bool prevDown=false; bool menuDirty=true;
bool chronosPreempt=false;

struct MenuItem { const char *icon; const char *label; uint16_t col; };
static const MenuItem MENU[] = {
  {">","GIF berikutnya",C_TEAL},{"T","Pilih tema",C_BAR},{"E","Ekspresi flash",C_PINK},
  {"M","Mode putar",C_YEL},{"R","Reaksi acak/tetap",C_BLUE},{"F","Model reaksi",C_PINK},
  {"S","Sumber SD/Flash",C_BLUE},{"+","Volume +",C_TEAL},{"-","Volume -",C_TEAL},
  {"x","Bisu / bunyi",C_RED},{"O","Rotasi layar",C_YEL},{"C","Chronos",C_TEAL},
  {"J","Jam HP",C_TEAL},{"W","Merek LCD",C_DIM},{"A","Info Wi-Fi AP",C_BLUE},{"i","Tentang rzmong",C_TEXT},
  {"P","Musik putar/jeda",C_TEAL},{"N","Lagu berikut",C_TEAL},{"B","Lagu sebelumnya",C_TEAL},{"<","Tutup",C_DIM}
};
static const int NMENU=20, VIS=7;
#include "chronos_ui.inc"

static bool validTheme(const String &t){ for(int i=0;i<MOCHI_THEME_COUNT;i++) if(t==MOCHI_THEMES[i]) return true; return false; }
static bool validMode(const String &v){ return v=="kategori"||v=="acak"||v=="acak_tema"; }
static bool validReact(const String &v){ return v=="acak"||v=="tetap"; }

void serviceNet();
void serviceSfx();
void stopSfx();
static void waitMs(uint32_t ms){
  uint32_t t=millis();
  while((uint32_t)(millis()-t)<ms){ serviceNet(); serviceSfx(); delay(10); }
}
static bool chronosNeedsScreen(){
  return chronosOn && (ringerOn || (notifUntil && (int32_t)(millis()-notifUntil)<0) || (chronosNav && navActive && !navHide));
}

void loadPrefs(){
  prefs.begin("rzmong",false);
  theme=prefs.getString("theme","wajah"); playMode=prefs.getString("mode","kategori");
  reactMode=prefs.getString("rmode","acak"); soundOn=prefs.getBool("sound",true);
  useSd=prefs.getBool("usesd",false); chronosOn=prefs.getBool("chrono",false); chronosNav=prefs.getBool("chrono_nav",true); showWm=prefs.getBool("wm",true); clockOn=prefs.getBool("clock",false);
  defIdx=prefs.getInt("def",0); reactIdx=prefs.getInt("react",0); volume=prefs.getInt("vol",12); rot=prefs.getInt("rot",MOCHI_DEFAULT_ROTATION)&3;
  // 0.5.4: LCD sekarang dipasang pin di bawah (putar 180°). NVS dari firmware lama (belum ada "rotv")
  // yang sudah menyimpan "rot" digeser sekali sebesar default baru (default lama = 0), setelan lain tidak disentuh.
  if(prefs.getUChar("rotv",0)<MOCHI_ROT_LAYOUT){
    if(prefs.isKey("rot")){ rot=(rot+MOCHI_DEFAULT_ROTATION)&3; prefs.putInt("rot",rot); }
    prefs.putUChar("rotv",MOCHI_ROT_LAYOUT);
  }
  if(defIdx<0||defIdx>=DEFAULT_GIF_COUNT)defIdx=0;
  if(reactIdx<0||reactIdx>=MOCHI_REACT_COUNT)reactIdx=0;
  if(volume<0)volume=0; if(volume>21)volume=21;
  if(!validTheme(theme)) theme="wajah";
  if(!validMode(playMode)) playMode="kategori";
  if(!validReact(reactMode)) reactMode="acak";
}
void savePrefs(){
  prefs.putString("theme",theme); prefs.putString("mode",playMode); prefs.putString("rmode",reactMode);
  prefs.putBool("sound",soundOn); prefs.putBool("usesd",useSd); prefs.putBool("chrono",chronosOn); prefs.putBool("chrono_nav",chronosNav); prefs.putBool("wm",showWm); prefs.putBool("clock",clockOn);
  prefs.putInt("def",defIdx); prefs.putInt("react",reactIdx); prefs.putInt("vol",volume); prefs.putInt("rot",rot);
}
static bool isGifName(const String &n){
  int sl=n.lastIndexOf('/'); String b=(sl<0)?n:n.substring(sl+1);
  if(b.length()==0||b[0]=='.') return false;
  b.toLowerCase(); return b.endsWith(".gif");
}
static String safeName(String s){
  s.trim(); if(s.length()>40) s=s.substring(0,40);
  for(size_t i=0;i<s.length();i++){ char c=s[i]; if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-')) s.setCharAt(i,'_'); }
  return s;
}
void scanTheme(const String &t){
  nparts=0; if(!sdOk||sdBusy)return;
  String dir=String("/gif/")+t; File d=SD.open(dir); if(!d)return;
  while(true){File f=d.openNextFile(); if(!f)break; String n=f.name(); f.close();
    if(isGifName(n)&&nparts<32){int sl=n.lastIndexOf('/'); parts[nparts++]=dir+"/"+n.substring(sl<0?0:sl+1);}}
  d.close();
  if(nparts<=0) idx=0; else if(idx<0||idx>=nparts) idx=0;
}
void GIFDraw(GIFDRAW *p){
  int cw=gif.getCanvasWidth(), ch=gif.getCanvasHeight();
  int sy=(H-ch)/2+p->iY+p->y; if(sy<0||sy>=H) return;
  int sx0=(W-cw)/2+p->iX;
  const uint16_t *pal=p->pPalette; uint8_t *s=p->pPixels; int n=p->iWidth;
  bool trans=p->ucHasTransparency; uint8_t tc=p->ucTransparent;
  if(trans && p->ucDisposalMethod==2){
    for(int i=0;i<n;i++) if(s[i]==tc) s[i]=p->ucBackground;
    trans=false;
  }
  uint16_t line[W]; int x=0;
  while(x<n){
    if(trans){ while(x<n && s[x]==tc) x++; if(x>=n) break; }
    int a=x; while(x<n && !(trans && s[x]==tc)) x++;
    int sx=sx0+a, len=x-a, off=0;
    if(sx<0){ off=-sx; sx=0; len-=off; }
    if(sx+len>W) len=W-sx;
    if(len<=0) continue;
    for(int i=0;i<len;i++) line[i]=pal[s[a+off+i]];
    tft.pushImage(sx,sy,len,1,line);
  }
}
void *gifOpen(const char *name,int32_t *sz){gifFile=SD.open(name); if(!gifFile)return NULL; *sz=gifFile.size(); return (void*)1;}
void gifClose(void*){if(gifFile)gifFile.close();}
int32_t gifRead(GIFFILE *p,uint8_t *buf,int32_t len){int n=gifFile.read(buf,len); p->iPos=gifFile.position(); return n;}
int32_t gifSeek(GIFFILE *p,int32_t pos){gifFile.seek(pos); p->iPos=gifFile.position(); return p->iPos;}

void audioInit(){
  i2sOk = mochiDfInit();
}

void stopSfx(){ mochiDfStop(); }
static bool playBuiltinSfx(int i){
  if(!soundOn) return false;
  mochiDfSetVolume(volume, true);
  return mochiDfPlayFace(i);
}
void serviceSfx(){ mochiDfService(); }
void startJingleMs(int ms){ (void)ms; mochiDfSetVolume(volume, soundOn); }
void playJingleMs(int ms){ startJingleMs(ms); }
bool playSfxForReact(int r){
  if(!soundOn) return false;
  mochiDfSetVolume(volume, true);
  if(mochiDfPlayReact(r)) return true;
  return mochiDfPlayFace(0);
}
bool playSfxForGifPath(const char *gifPath){
  if(!soundOn||!gifPath) return false;
  mochiDfSetVolume(volume, true);
  return mochiDfPlayGif(gifPath);
}

int pickReact(){ return (reactMode=="acak") ? random(MOCHI_REACT_COUNT) : reactIdx; }
bool playOpen(const uint8_t *mem,int len,const char *path){
  if(mem) return gif.open((uint8_t*)mem,len,GIFDraw);
  if(!sdOk) return false;
  return gif.open(path,gifOpen,gifClose,gifRead,gifSeek,GIFDraw);
}
static bool frameWait(int delayMs, uint32_t t0, uint32_t maxMs){
  uint32_t until=millis()+(delayMs<1?1:delayMs);
  while((int32_t)(until-millis())>0){
    serviceNet();
    if(chronosNeedsScreen()){ chronosPreempt=true; stopSfx(); return true; }
    if(maxMs && (uint32_t)(millis()-t0)>=maxMs) return true;
    bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
    if(down&&!prevDown) downAt=millis();
    if(!down&&prevDown){
      uint32_t held=millis()-downAt;
      if(held>=900){ soundOn=!soundOn; if(!soundOn) stopSfx(); else mochiDfSetVolume(volume, true); savePrefs(); menuDirty=true; }
      else { taps++; lastTap=millis(); }
      prevDown=down; return true;
    }
    prevDown=down;
    delay(1);
  }
  return false;
}
static bool playFrames(uint32_t maxMs){
  uint32_t t0=millis();
  while(true){
    int dly=0;
    if(!gif.playFrame(false, &dly)) return false;
    if(frameWait(dly, t0, maxMs)) return true;
    if(maxMs && (uint32_t)(millis()-t0)>=maxMs) return true;
  }
}
void serviceNet(){
  dnsServer.processNextRequest();
  server.handleClient();
  if(chronosOn) watch.loop();
  serviceSfx();
  if(chronosOn && watch.isRunning()) chronoConn=watch.isConnected();
  yield();
}
void playReactGif(){
  int r=pickReact(); playSfxForReact(r);
  if(chronosNeedsScreen()){ chronosPreempt=true; return; }
  String path=String("/gif/")+MOCHI_REACT[r].theme+"/"+MOCHI_REACT[r].stem+".gif";
  bool ok=false;
  if(sdOk && !sdBusy && SD.exists(path.c_str())){ sdBusy=true; ok=playOpen(NULL,0,path.c_str()); if(!ok) sdBusy=false; }
  if(!ok){ int bi=findBuiltin(MOCHI_REACT[r].theme,MOCHI_REACT[r].stem); if(bi>=0) ok=playOpen(DEFAULT_GIFS[bi].data,DEFAULT_GIFS[bi].len,NULL); }
  if(!ok){ sdBusy=false; return; }
  tft.fillScreen(TFT_BLACK); playFrames(1600);
  gif.close(); sdBusy=false;
}
void brandMark(){if(!showWm)return; tft.setTextColor(C_DIM,TFT_BLACK); tft.drawString(MOCHI_BRAND,168,226,1);}
static const uint8_t SEG7[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
static int clockDrawn=-1;
static bool clockGifOn=false;
static const char *WD[7]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
static void eye(int cx, int cy, int open){
  tft.fillRect(cx-28, cy-20, 56, 42, TFT_BLACK);
  tft.fillRoundRect(cx-24, cy-8, 48, 20, 8, TFT_WHITE);
  if(open<=0){
    tft.fillRoundRect(cx-24, cy-8, 48, 16, 6, TFT_BLACK);
    tft.drawWideLine(cx-22, cy+2, cx+22, cy+2, 2, TFT_WHITE, TFT_BLACK);
    return;
  }
  int lid=18-open*3; if(lid<0) lid=0;
  tft.fillCircle(cx, cy+2, 7, 0x4A69);
  tft.fillCircle(cx, cy+2, 3, TFT_BLACK);
  tft.fillCircle(cx-2, cy, 1, TFT_WHITE);
  if(lid>0) tft.fillRoundRect(cx-24, cy-10, 48, lid, 4, TFT_BLACK);
}
static void blob(int x,int y,int w,int h){ tft.fillRoundRect(x,y,w,h,h/2,TFT_WHITE); }
static void ring(int x,int y,int w,int h,int t){
  tft.fillRoundRect(x,y,w,h,h/3,TFT_WHITE);
  tft.fillRoundRect(x+t,y+t,w-2*t,h-2*t,(h-2*t)/3,TFT_BLACK);
}
static void digitR(int x,int y,int d){
  int w=36,h=46,t=8;
  if(d==0) ring(x,y,w,h,t);
  else if(d==1) blob(x+w-t-2,y,t,h);
  else if(d==2){ ring(x,y,w,h/2+2,t); blob(x,y+h-t,w,t); blob(x,y+h/2-2,t,h/2); }
  else if(d==3){ ring(x,y,w,h/2+2,t); ring(x,y+h/2-2,w,h/2+2,t); }
  else if(d==4){ blob(x,y,t,h/2+2); blob(x,y+h/2-t/2,w,t); blob(x+w-t,y,t,h); }
  else if(d==5){ blob(x,y,w,t); blob(x,y,t,h/2); ring(x,y+h/2-2,w,h/2+2,t); }
  else if(d==6){ blob(x,y,t,h); ring(x,y+h/2-2,w,h/2+2,t); blob(x,y,w,t); }
  else if(d==7){ blob(x,y,w,t); blob(x+w-t,y,t,h); }
  else if(d==8){ ring(x,y,w,h/2+2,t); ring(x,y+h/2-2,w,h/2+2,t); }
  else ring(x,y,w,h/2+2,t), blob(x+w-t,y,t,h), blob(x,y+h-t,w,t);
}
void drawClock(){
  bool linked=chronosOn && watch.isRunning() && watch.isConnected();
  int h=linked?watch.getHourC():0, m=linked?watch.getMinute():0, s=linked?watch.getSecond():0;
  int sig=linked?(h*3600+m*60+s):-2;
  if(sig==clockDrawn && !clockGifOn) return;
  clockDrawn=sig;
  if(!clockGifOn) tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  if(linked){
    int wd=watch.getDayofWeek(); if(wd<0||wd>6) wd=0;
    char top[8]; snprintf(top,sizeof(top),"%02d", watch.getDay());
    tft.drawCentreString(top,70,8,4);
    tft.drawCentreString(WD[wd],170,14,2);
    int x=18, y=36, gap=6;
    digitR(x,y,h/10); x+=36+gap;
    digitR(x,y,h%10); x+=36+2;
    uint16_t col=(s&1)?TFT_WHITE:TFT_BLACK;
    tft.fillCircle(x+4, y+14, 3, col); tft.fillCircle(x+4, y+30, 3, col);
    x+=14;
    digitR(x,y,m/10); x+=36+gap;
    digitR(x,y,m%10);
  } else {
    tft.drawCentreString("--",70,8,4);
    tft.drawCentreString("---",170,14,2);
    tft.drawCentreString("--:--",120,48,4);
  }
  if(!clockGifOn){ int blink=(s%5==0)?0:(s%5==1)?2:6; eye(78,132,blink); eye(162,132,blink); }
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  if(linked){
    char nb[8]; snprintf(nb,sizeof(nb),"%d", watch.getPhoneBattery());
    tft.drawCentreString(nb,120,176,4);
  } else tft.drawCentreString("HP",120,176,4);
  tft.setTextColor(0x4A69,TFT_BLACK);
  tft.drawCentreString("2x menu",120,214,1);
}
void bootMark(){
  tft.fillScreen(C_BG); tft.fillRoundRect(20,80,200,80,16,C_BAR);
  tft.setTextColor(TFT_BLACK,C_BAR); tft.drawCentreString("Mochi",120,96,4); tft.drawCentreString(MOCHI_BRAND,120,128,2); waitMs(400);
}
void drawMenu(){
  if(menuRow<menuTop) menuTop=menuRow; if(menuRow>=menuTop+VIS) menuTop=menuRow-VIS+1;
  tft.fillScreen(C_BG); tft.fillRect(0,0,240,34,C_BAR);
  tft.setTextColor(TFT_BLACK,C_BAR); tft.drawCentreString("PENGATURAN",120,4,2); tft.drawCentreString("rzmong",120,20,1);
  for(int i=0;i<VIS;i++){
    int id=menuTop+i; if(id>=NMENU)break; int y=40+i*26;
    if(id==menuRow){tft.fillRoundRect(6,y-2,228,25,6,C_SEL); tft.fillCircle(20,y+10,7,MENU[id].col); tft.setTextColor(TFT_BLACK,C_SEL);}
    else {tft.fillCircle(20,y+10,6,MENU[id].col); tft.setTextColor(C_TEXT,C_BG);}
    tft.drawString(String(MENU[id].icon)+" "+MENU[id].label,34,y+4,2);
  }
  tft.setTextColor(C_DIM,C_BG);
  char foot[80]; snprintf(foot,sizeof(foot),"%d/%d vol%d %s %s%s",menuRow+1,NMENU,volume,soundOn?"snd":"bisu",useSd&&sdOk?"SD":"flash", chronosOn?(chronoConn?" BLE*":" BLE"):"");
  tft.drawString(foot,8,224,1);
}
void nextPart(){
  if(useSd&&sdOk){
    if(playMode=="acak"){
      int st=random(MOCHI_THEME_COUNT);   // jangan timpa theme, supaya NVS tidak menyimpan tema acak
      for(int k=0;k<MOCHI_THEME_COUNT;k++){ scanTheme(MOCHI_THEMES[(st+k)%MOCHI_THEME_COUNT]); if(nparts>0) break; }
    } else if(nparts==0) scanTheme(theme);
    if(nparts>0){ idx=(playMode=="acak"||playMode=="acak_tema")?random(nparts):(idx+1)%nparts; return; }
  }
  static int rotI=0;   // wajah default (slot 0) diselang tiap klip lain
  if(DEFAULT_GIF_COUNT>1 && defIdx==0){ rotI=rotI%(DEFAULT_GIF_COUNT-1)+1; defIdx=rotI; } else defIdx=0;   // tidak savePrefs, tidak menimpa theme, tidak mematikan useSd
}
void showInfo(const char *a,const char *b){
  tft.fillScreen(C_BG); tft.fillRoundRect(16,70,208,100,14,C_SEL);
  tft.setTextColor(TFT_BLACK,C_SEL); tft.drawCentreString(a,120,90,2); tft.drawCentreString(b,120,118,2); waitMs(800);
}
void applyMenu(){
  if(menuRow==0){nextPart(); ui=UI_PLAY;}
  else if(menuRow==1){
    int i=0; for(;i<MOCHI_THEME_COUNT;i++) if(theme==MOCHI_THEMES[i]) break;
    theme=MOCHI_THEMES[(i+1)%MOCHI_THEME_COUNT];
    if(sdOk){ useSd=true; scanTheme(theme); idx=0; } savePrefs();
    if(soundOn) mochiDfPlayTheme(theme.c_str());
    if(sdOk && nparts>0) showInfo(theme.c_str(), (String(nparts)+" GIF").c_str());
    else if(sdOk) showInfo(theme.c_str(),"kosong di SD"); else showInfo(theme.c_str(),"default (no SD)");
  }
  else if(menuRow==2){defIdx=(defIdx+1)%DEFAULT_GIF_COUNT; savePrefs(); if(soundOn){ mochiDfSetVolume(volume,true); mochiDfPlayFace(defIdx); } showInfo("ekspresi",DEFAULT_GIFS[defIdx].stem);}
  else if(menuRow==3){playMode=(playMode=="kategori")?"acak":(playMode=="acak"?"acak_tema":"kategori"); savePrefs(); showInfo("mode",playMode.c_str());}
  else if(menuRow==4){reactMode=(reactMode=="acak")?"tetap":"acak"; savePrefs(); showInfo("reaksi",reactMode.c_str());}
  else if(menuRow==5){reactIdx=(reactIdx+1)%MOCHI_REACT_COUNT; reactMode="tetap"; savePrefs(); if(soundOn) playSfxForReact(reactIdx); showInfo(MOCHI_REACT[reactIdx].name,MOCHI_REACT[reactIdx].stem);}
  else if(menuRow==6){
    if(!sdOk){ useSd=false; savePrefs(); showInfo("sumber","SD tidak ada"); }
    else { useSd=!useSd; if(useSd){ scanTheme(theme); idx=0; } savePrefs(); showInfo("sumber", useSd?(nparts?"SD":"SD kosong"):"flash"); }
  }
  else if(menuRow==7){if(volume<21)volume++; savePrefs(); showInfo("volume",String(volume).c_str()); startJingleMs(120);}
  else if(menuRow==8){if(volume>0)volume--; savePrefs(); showInfo("volume",String(volume).c_str()); startJingleMs(120);}
  else if(menuRow==9){soundOn=!soundOn; if(!soundOn) stopSfx(); else mochiDfSetVolume(volume, true); savePrefs(); showInfo("suara",soundOn?"ON":"BISU");}
  else if(menuRow==10){rot=(rot+1)&3; tft.setRotation(rot); savePrefs();}
  else if(menuRow==11){chronosOn=!chronosOn; savePrefs(); chronosApply(); showInfo("Chronos",chronosOn?(chronoConn?"ON linked":"ON pair app"):"OFF");}
  else if(menuRow==12){clockOn=!clockOn; if(clockOn && !chronosOn){ chronosOn=true; chronosApply(); } clockDrawn=-1; savePrefs(); showInfo("jam HP", clockOn?(chronoConn?"waktu HP":"menunggu HP"):"GIF");}
  else if(menuRow==13){showWm=!showWm; savePrefs();}
  else if(menuRow==14){showInfo(MOCHI_AP_NAME, MOCHI_AP_PASS);}
  else if(menuRow==15){bootMark();}
  else if(menuRow==16){ if(soundOn){ mochiDfSetVolume(volume,true); mochiDfMusicToggle(); } showInfo("musik", mochiDfMusicPlaying()?"putar":"jeda"); }
  else if(menuRow==17){ if(soundOn){ mochiDfSetVolume(volume,true); mochiDfMusicNext(); } showInfo("musik","berikut"); }
  else if(menuRow==18){ if(soundOn){ mochiDfSetVolume(volume,true); mochiDfMusicPrev(); } showInfo("musik","sebelum"); }
  else ui=UI_PLAY;
}
void finishTaps(){
  if(!taps||millis()-lastTap<=320)return;
  if(ui==UI_MENU){ if(taps==1) menuRow=(menuRow+1)%NMENU; else applyMenu(); menuDirty=true; }
  else {
    if(taps>=2){ ui=UI_MENU; menuRow=0; menuTop=0; taps=0; drawMenu(); menuDirty=false; return; }
    if(taps==1 && !clockOn) playReactGif();
  }
  taps=0;
}
bool playCurrent(){
  if(taps) return true;
  if(nparts<=0) idx=0; else if(idx<0||idx>=nparts) idx=0;
  bool ok;
  bool fromSd=useSd&&sdOk&&nparts>0;
  if(fromSd && soundOn && !mochiDfMusicPlaying()) playSfxForGifPath(parts[idx].c_str());
  else if(!fromSd && soundOn && !mochiDfMusicPlaying()){
    // wajah/default (slot 0) diselang tiap klip: bunyinya hanya sekali sesudah boot, supaya tidak bip berulang
    static bool defaultSfxDone=false;
    if(defIdx!=0) playBuiltinSfx(defIdx);
    else if(!defaultSfxDone){ playBuiltinSfx(0); defaultSfxDone=true; }
  }
  if(chronosNeedsScreen()){ chronosPreempt=true; return true; }
  if(fromSd){ sdBusy=true; ok=playOpen(NULL,0,parts[idx].c_str()); }
  else ok=playOpen(DEFAULT_GIFS[defIdx].data,DEFAULT_GIFS[defIdx].len,NULL);
  if(!ok){
    sdBusy=false;
    ok=playOpen(DEFAULT_GIFS[defIdx].data,DEFAULT_GIFS[defIdx].len,NULL);
    if(!ok){ delay(40); return false; }
  }
  tft.fillScreen(TFT_BLACK);
  playFrames(0);
  gif.close(); sdBusy=false; brandMark();
  if(taps==0 && !chronosPreempt) nextPart();
  return true;
}
void sendCorsHeaders(){
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Access-Control-Allow-Methods","GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers","Content-Type");
}
void handleOptions(){ sendCorsHeaders(); server.send(204); }
void handleRoot(){
  server.sendHeader("Cache-Control","no-store");
  server.send_P(200,"text/html; charset=utf-8",CAPTIVE_HTML);
}
void handleCaptiveProbe(){
  server.sendHeader("Cache-Control","no-store");
  server.send_P(200,"text/html; charset=utf-8",CAPTIVE_HTML);
}
void handleNotFound(){
  server.sendHeader("Location", String("http://")+WiFi.softAPIP().toString()+"/", true);
  server.send(302,"text/plain","");
}
void handleStatus(){
  JsonDocument d; d["brand"]=MOCHI_BRAND; d["ver"]=MOCHI_VERSION; d["theme"]=theme;
  d["mode"]=playMode; d["react_mode"]=reactMode; d["react"]=MOCHI_REACT[reactIdx].name;
  d["react_idx"]=reactIdx;
  d["react_gif"]=String("/gif/")+MOCHI_REACT[reactIdx].theme+"/"+MOCHI_REACT[reactIdx].stem+".gif";
  d["sound"]=soundOn; d["vol"]=volume; d["rot"]=rot; d["storage"]=(useSd&&sdOk)?"sd":"flash"; d["sd"]=sdOk;
  d["sfx"]="dfplayer_mp3"; d["music"]=mochiDfMusicPlaying(); d["def"]=defIdx; d["gif_count"]=nparts;
  d["ap_ssid"]=MOCHI_AP_NAME; d["sd_busy"]=sdBusy;
  d["chronos"]=chronosOn;
  d["chronos_conn"]=chronosOn && watch.isRunning() && watch.isConnected();
  d["chronos_run"]=chronosOn && watch.isRunning();
  d["chronos_nav"]=chronosNav;
  d["clock"]=clockOn;
  d["nav_active"]=navActive && !navHide;
  if(chronosOn && watch.isRunning()){
    d["chronos_mac"]=watch.getAddress();
    char tb[8]; snprintf(tb,sizeof(tb),"%02d:%02d", watch.getHourC(), watch.getMinute());
    d["chronos_time"]=tb;
  }
  if(navActive){ d["nav_title"]=navTitle; d["nav_dist"]=navDist; }
  if(notifApp.length()){ d["notif_app"]=notifApp; d["notif_title"]=notifTitle; d["notif_msg"]=notifMsg; }
  if(ringerOn) d["ringer"]=ringerName;
  JsonArray themes=d["themes"].to<JsonArray>();
  for(int i=0;i<MOCHI_THEME_COUNT;i++) themes.add(MOCHI_THEMES[i]);
  String s; serializeJson(d,s); sendCorsHeaders(); server.send(200,"application/json",s);
}
void handleThemes(){
  if(sdBusy){ sendCorsHeaders(); server.send(503,"application/json","{\"error\":\"sd_busy\"}"); return; }
  JsonDocument d; d["sd"]=sdOk; d["current"]=theme; d["storage"]=(useSd&&sdOk)?"sd":"flash";
  JsonArray arr=d["themes"].to<JsonArray>();
  for(int i=0;i<MOCHI_THEME_COUNT;i++){
    JsonObject o=arr.add<JsonObject>(); o["id"]=MOCHI_THEMES[i]; int cnt=0;
    if(sdOk){ String dir=String("/gif/")+MOCHI_THEMES[i]; File dd=SD.open(dir);
      if(dd){ while(true){File f=dd.openNextFile(); if(!f)break; String n=f.name(); f.close(); if(isGifName(n)) cnt++; } dd.close(); }
    }
    o["gif_count"]=cnt; o["available"]=(cnt>0)||(!sdOk && i==0);
  }
  String s; serializeJson(d,s); sendCorsHeaders(); server.send(200,"application/json",s);
}

void handleMusic(){
  if(!server.hasArg("plain")){ server.send(400,"application/json","{\"ok\":false}"); return; }
  JsonDocument d; if(deserializeJson(d, server.arg("plain"))){ server.send(400,"application/json","{\"ok\":false}"); return; }
  const char *act = d["action"] | "";
  mochiDfSetVolume(volume, soundOn);
  bool ok=false;
  if(!strcmp(act,"play")) ok=mochiDfMusicStart();
  else if(!strcmp(act,"next")) ok=mochiDfMusicNext();
  else if(!strcmp(act,"prev")) ok=mochiDfMusicPrev();
  else if(!strcmp(act,"toggle")){ mochiDfMusicToggle(); ok=true; }
  else if(!strcmp(act,"stop")){ mochiDfMusicStop(); ok=true; }
  JsonDocument o; o["ok"]=ok; o["music"]=mochiDfMusicPlaying();
  String body; serializeJson(o, body); sendCorsHeaders(); server.send(200,"application/json", body);
}

void handleSettings(){
  JsonDocument d; if(deserializeJson(d,server.arg("plain"))){ server.send(400,"text/plain","bad"); return; }
  if(d["theme"].is<const char*>()){ String t=(const char*)d["theme"]; if(validTheme(t)) theme=t; }
  if(d["play_mode"].is<const char*>()){ String v=(const char*)d["play_mode"]; if(validMode(v)) playMode=v; }
  if(d["react_mode"].is<const char*>()){ String v=(const char*)d["react_mode"]; if(validReact(v)) reactMode=v; }
  if(d["sound"].is<bool>()) soundOn=d["sound"];
  if(d["chronos"].is<bool>()){ chronosOn=d["chronos"]; chronosApply(); }
  if(d["chronos_nav"].is<bool>()){ chronosNav=d["chronos_nav"]; }
  if(d["clock"].is<bool>()){ clockOn=d["clock"]; if(clockOn && !chronosOn){ chronosOn=true; chronosApply(); } }
  if(d["watermark"].is<bool>()) showWm=d["watermark"];
  if(d["storage"].is<const char*>()) useSd=(String((const char*)d["storage"])=="sd");
  if(d["def"].is<int>()){ defIdx=d["def"]; if(defIdx<0||defIdx>=DEFAULT_GIF_COUNT) defIdx=0; }
  if(d["react"].is<int>()){ reactIdx=d["react"]; if(reactIdx<0||reactIdx>=MOCHI_REACT_COUNT) reactIdx=0; }
  if(d["volume"].is<int>()){ volume=d["volume"]; if(volume<0)volume=0; if(volume>21)volume=21; }
  if(sdOk && d["theme"].is<const char*>() && !d["storage"].is<const char*>()) useSd=true;
  if(useSd && !sdOk) useSd=false;
  savePrefs(); if(useSd && sdOk && !sdBusy){ scanTheme(theme); idx=0; }
  JsonDocument out; out["ok"]=true; out["brand"]=MOCHI_BRAND; out["theme"]=theme;
  out["storage"]=(useSd&&sdOk)?"sd":"flash"; out["gif_count"]=nparts; out["sd"]=sdOk;
  String s; serializeJson(out,s); sendCorsHeaders(); server.send(200,"application/json",s);
}

static void releaseSdPlayback(){
  if(gifFile) gifFile.close();
  gif.close();
  sdBusy=false;
  clockGifOn=false;
}
static File upFile;
static String upPath;
static bool upOk=false;
static size_t upWritten=0;
static int upHttp=400;
static String upJson;
static bool upOwnsBusy=false;
static const size_t UPLOAD_MAX=600000;

void handleUpload() {
  HTTPUpload& upload=server.upload();
  if(upload.status==UPLOAD_FILE_START){
    upOk=false; upWritten=0; upPath=""; upHttp=400; upJson="{\"error\":\"upload_failed\"}";
    if(!sdOk){ upJson="{\"error\":\"sd_not_ready\"}"; return; }
    if(sdBusy) releaseSdPlayback();
    sdBusy=true; upOwnsBusy=true;
    String tema=server.arg("tema"); String stem=server.arg("stem"); String type=server.arg("type");
    tema.trim(); stem.trim(); type.trim(); type.toLowerCase();
    tema=safeName(tema); stem=safeName(stem);
    if(!validTheme(tema)||stem.length()==0||(type!="gif"&&type!="wav")){
      upJson="{\"error\":\"bad_name\"}"; sdBusy=false; upOwnsBusy=false; return;
    }
    String dir=(type=="gif")?String("/gif/")+tema:String("/sfx/")+tema;
    if(!SD.exists("/gif")) SD.mkdir("/gif");
    if(!SD.exists("/sfx")) SD.mkdir("/sfx");
    if(!SD.exists(dir)) SD.mkdir(dir);
    upPath=dir+"/"+stem+"."+type;
    if(SD.exists(upPath)) SD.remove(upPath);
    upFile=SD.open(upPath, FILE_WRITE);
    upOk=(bool)upFile;
  } else if(upload.status==UPLOAD_FILE_WRITE){
    if(upOk && upFile){
      if(upWritten+upload.currentSize>UPLOAD_MAX){ upFile.close(); SD.remove(upPath); upOk=false; upPath=""; upJson="{\"error\":\"too_large\"}"; }
      else {
        size_t w=upFile.write(upload.buf, upload.currentSize); upWritten+=w;
        if(w!=upload.currentSize){ upFile.close(); SD.remove(upPath); upOk=false; upPath=""; }
      }
    }
  } else if(upload.status==UPLOAD_FILE_END){
    if(upOk && upFile){
      upFile.close();
      JsonDocument out; out["ok"]=true; out["path"]=upPath; out["size"]=(int)upWritten; out["brand"]=MOCHI_BRAND;
      serializeJson(out, upJson); upHttp=200;
    } else {
      if(upFile) upFile.close();
      if(upPath.length()) SD.remove(upPath);
      if(!upJson.length()) upJson=sdOk?"{\"error\":\"upload_failed\"}":"{\"error\":\"sd_not_ready\"}";
      upHttp=400;
    }
    if(upOwnsBusy) sdBusy=false; upOwnsBusy=false; upPath=""; upOk=false; upWritten=0;
  } else if(upload.status==UPLOAD_FILE_ABORTED){
    if(upFile) upFile.close();
    if(upPath.length()) SD.remove(upPath);
    if(upOwnsBusy) sdBusy=false; upOwnsBusy=false; upPath=""; upOk=false; upWritten=0; upHttp=400; upJson="{\"error\":\"aborted\"}";
  }
}
void handleUploadDone(){
  sendCorsHeaders();
  server.send(upHttp?upHttp:400, "application/json", upJson.length()?upJson:"{\"error\":\"upload_failed\"}");
  if(sdOk && !sdBusy) scanTheme(theme);
}

void setup(){
  pinMode(MOCHI_PIN_SD_CS,OUTPUT); digitalWrite(MOCHI_PIN_SD_CS,HIGH);
  Serial.begin(115200); pinMode(MOCHI_PIN_TOUCH,INPUT_PULLDOWN); loadPrefs();
  gif.begin(GIF_PALETTE_RGB565_BE);
  // Satu FSPI: klaim MISO SD dulu, TFT menyusul. CS SD tetap HIGH supaya modul tidak nimbrung.
  SPI.begin(MOCHI_PIN_SD_SCK,MOCHI_PIN_SD_MISO,MOCHI_PIN_SD_MOSI,MOCHI_PIN_SD_CS);
  tft.init(); tft.setRotation(rot); bootMark();
  sdOk=SD.begin(MOCHI_PIN_SD_CS,SPI,4000000); if(sdOk) scanTheme(theme);
  tft.setRotation(rot);
  if(useSd&&(!sdOk||nparts==0)) useSd=false;
  audioInit();
  apPass=MOCHI_AP_PASS;
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(MOCHI_AP_NAME, apPass);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/",handleRoot);
  server.on("/index.html",handleRoot);
  server.on("/generate_204",handleCaptiveProbe);
  server.on("/hotspot-detect.html",handleCaptiveProbe);
  server.on("/ncsi.txt",handleCaptiveProbe);
  server.on("/api/status",handleStatus);
  server.on("/api/themes",handleThemes);
  server.on("/api/settings",HTTP_POST,handleSettings);
  server.on("/api/music", HTTP_POST, handleMusic);
  server.on("/api/music", HTTP_OPTIONS, handleOptions);
  server.on("/api/upload", HTTP_POST, handleUploadDone, handleUpload);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/themes", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/upload", HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);
  server.begin();
  chronosSetupCallbacks();
  if(chronosOn) chronosApply();
}
void loop(){
  serviceNet();
  if(chronosOn && findUntil){ serviceChronosFind(); menuDirty=true; delay(30); return; }
  static bool ringerDrew=false;
  if(!(chronosOn && ringerOn)) ringerDrew=false;
  if(chronosOn && ringerOn){
    if(!ringerDrew){ drawChronosRinger(); ringerDrew=true; menuDirty=true; }
    stopSfx();
    bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
    if(down&&!prevDown) downAt=millis();
    if(!down&&prevDown && millis()-downAt>=600){ ringerOn=false; ringerDrew=false; mochiDfPlayRinger(false); }
    prevDown=down; delay(30); return;
  }
  if(chronosOn && notifUntil && (int32_t)(millis()-notifUntil)<0){
    static uint32_t drewUntil=0;
    if(drewUntil!=notifUntil){ drawChronosNotif(); drewUntil=notifUntil; }
    menuDirty=true;
    bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
    if(down&&!prevDown) notifUntil=0;
    prevDown=down; delay(30); return;
  } else if(notifUntil && (int32_t)(millis()-notifUntil)>=0){
    notifUntil=0;
  }
  static bool navDrew=false; static String navSig;
  if(!(chronosOn && chronosNav && navActive && !navHide)) navDrew=false;
  if(chronosOn && chronosNav && navActive && !navHide){
    String sig=navTitle+"|"+navDist;
    if(!navDrew || sig!=navSig){ drawChronosNav(); navDrew=true; navSig=sig; menuDirty=true; }
    bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
    if(down&&!prevDown){
      if(millis()-lastTap<400){ navHide=true; taps=0; navDrew=false; }
      else lastTap=millis();
    }
    prevDown=down; delay(30); return;
  }
  chronosPreempt=false;
  if(!clockOn && clockGifOn){ gif.close(); clockGifOn=false; sdBusy=false; }
  if(clockOn){
    serviceNet();
    if(chronosNeedsScreen()){ if(clockGifOn){ gif.close(); clockGifOn=false; sdBusy=false; } delay(1); return; }
    if(!clockGifOn){
      const char *path="/gif/wajah/default.gif";
      bool ok=false;
      if(sdOk && !sdBusy && SD.exists(path)){ sdBusy=true; ok=playOpen(NULL,0,path); if(!ok) sdBusy=false; }
      if(!ok){ int bi=findBuiltin("wajah","default"); if(bi>=0) ok=playOpen(DEFAULT_GIFS[bi].data,DEFAULT_GIFS[bi].len,NULL); }
      if(!ok && DEFAULT_GIF_COUNT>0) ok=playOpen(DEFAULT_GIFS[0].data,DEFAULT_GIFS[0].len,NULL);
      clockGifOn=ok;
    }
    static uint32_t clockNext=0; int dly=30;
    if(clockGifOn && (int32_t)(millis()-clockNext)>=0){
      if(!gif.playFrame(false,&dly)){ gif.close(); clockGifOn=false; sdBusy=false; dly=30; }
      clockNext=millis()+(dly<10?10:dly);
    }
    drawClock();
    bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
    if(down&&!prevDown) downAt=millis();
    if(!down&&prevDown){
      uint32_t held=millis()-downAt;
      if(held>=900){ soundOn=!soundOn; if(!soundOn) stopSfx(); else mochiDfSetVolume(volume, true); savePrefs(); }
      else { taps++; lastTap=millis(); }
    }
    prevDown=down; finishTaps(); delay(10); return;
  }
  bool down=digitalRead(MOCHI_PIN_TOUCH)==HIGH;
  if(ui==UI_MENU){
    if(down&&!prevDown) downAt=millis();
    if(!down&&prevDown){
      uint32_t held=millis()-downAt;
      if(held>=900){ soundOn=!soundOn; if(!soundOn) stopSfx(); else mochiDfSetVolume(volume, true); savePrefs(); menuDirty=true; }
      else { taps++; lastTap=millis(); }
    }
    prevDown=down; finishTaps(); if(menuDirty){ drawMenu(); menuDirty=false; } delay(35); return;
  }
  if(taps){
    if(down&&!prevDown) downAt=millis();
    if(!down&&prevDown){
      uint32_t held=millis()-downAt;
      if(held>=900){ soundOn=!soundOn; if(!soundOn) stopSfx(); else mochiDfSetVolume(volume, true); savePrefs(); }
      else { taps++; lastTap=millis(); }
    }
    prevDown=down; finishTaps(); delay(10); return;
  }
  finishTaps(); playCurrent();
}
