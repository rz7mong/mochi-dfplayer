uint32_t findUntil=0;

// chronos_ui.inc — Chronos full + nav (dirty redraw)
// Touch: Notif=1x · Ringer=hold 0.6s · Nav=2x hide · Menu/play unchanged
void playJingleMs(int ms);

bool navHide = false;
uint32_t navIconCRC = 0xFFFFFFFF;
String navTitle, navDir, navDist, navEta, navSpeed;
bool navActive = false;
bool navDirty = true;

void drawChronosNotif(){
  tft.fillScreen(C_BG);
  tft.fillRect(0,0,240,36,C_TEAL);
  tft.setTextColor(TFT_BLACK,C_TEAL);
  tft.drawCentreString(notifApp.length()?notifApp.c_str():"Notifikasi",120,8,2);
  tft.setTextColor(C_TEXT,C_BG);
  tft.drawString(notifTitle,12,50,2);
  tft.setTextColor(C_DIM,C_BG);
  String m=notifMsg;
  if(m.length()>36){ tft.drawString(m.substring(0,36),12,80,1); tft.drawString(m.substring(36,72),12,96,1); }
  else tft.drawString(m,12,80,1);
  tft.setTextColor(C_DIM,C_BG);
  tft.drawCentreString("ketuk 1x = tutup",120,220,1);
  if(chronoConn){
    char tbuf[24]="--:--";
    String td=watch.getTimeDate();
    if(td.length()>=5) td.substring(0,5).toCharArray(tbuf,24);
    tft.drawCentreString(tbuf,120,200,2);
  }
}
void drawChronosRinger(){
  tft.fillScreen(C_BG);
  tft.fillRoundRect(20,60,200,100,16,C_RED);
  tft.setTextColor(TFT_WHITE,C_RED);
  tft.drawCentreString("Panggilan",120,80,2);
  tft.drawCentreString(ringerName.length()?ringerName.c_str():"...",120,110,2);
  tft.setTextColor(C_DIM,C_BG);
  tft.drawCentreString("tahan 0.6s = tutup",120,200,1);
}
void drawChronosNav(){
  if(!navDirty) return;
  navDirty = false;
  tft.fillScreen(C_BG);
  tft.fillRect(0,0,240,28,C_BLUE);
  tft.setTextColor(TFT_WHITE,C_BLUE);
  tft.drawCentreString("Navigasi",120,6,2);
  Navigation &nv = watch.getNavigation();
  if(nv.hasIcon){
    int ox = (240-48)/2, oy = 36;
    for(int y=0;y<48;y++){
      for(int x=0;x<48;x++){
        int bi = (y*48+x)/8;
        int bp = 7-(x%8);
        bool on = (nv.icon[bi]>>bp)&1;
        if(on) tft.drawPixel(ox+x, oy+y, C_YEL);
      }
    }
  }
  tft.setTextColor(C_TEXT,C_BG);
  if(navTitle.length()) tft.drawCentreString(navTitle.substring(0,22),120,90,2);
  tft.setTextColor(C_TEAL,C_BG);
  if(navDist.length()) tft.drawCentreString(navDist.substring(0,20),120,118,2);
  tft.setTextColor(C_DIM,C_BG);
  String d = navDir;
  if(d.length()>40){
    tft.drawString(d.substring(0,40),8,148,1);
    tft.drawString(d.substring(40,80),8,162,1);
  } else if(d.length()) tft.drawString(d,8,148,1);
  if(navEta.length()) tft.drawCentreString(String("ETA ")+navEta.substring(0,16),120,186,1);
  if(navSpeed.length()) tft.drawCentreString(navSpeed.substring(0,12),120,202,1);
  tft.drawCentreString("ketuk 2x = sembunyi",120,222,1);
}
void refreshNavFromWatch(){
  Navigation &nv = watch.getNavigation();
  navActive = nv.active;
  if(!nv.active){ navHide=false; navDirty=true; return; }
  navDirty = true;
  navTitle = nv.title; if(navTitle.length()>28) navTitle=navTitle.substring(0,28);
  navDir = nv.directions; if(navDir.length()>80) navDir=navDir.substring(0,80);
  navDist = nv.distance;
  navEta = nv.eta;
  navSpeed = nv.speed;
}
void onChronosConn(bool state){
  chronoConn=state;
  Serial.printf("[chronos] %s\n", state?"connected":"disconnected");
  if(state) watch.setBattery(100, false);
  if(!state){ navActive=false; ringerOn=false; }
}
void onChronosNotif(Notification n){
  notifApp=n.app; if(notifApp.length()>22) notifApp=notifApp.substring(0,22);
  notifTitle=n.title; if(notifTitle.length()>28) notifTitle=notifTitle.substring(0,28);
  notifMsg=n.message; if(notifMsg.length()>72) notifMsg=notifMsg.substring(0,72);
  notifUntil=millis()+6000;
  playJingleMs(180);
}
void onChronosRinger(String caller, bool state){
  ringerOn=state;
  ringerName=caller; if(ringerName.length()>24) ringerName=ringerName.substring(0,24);
  if(state) playJingleMs(500);
}
void onChronosConfig(Config config, uint32_t a, uint32_t b){
  if(config==CF_FIND){
    playJingleMs(700);
    findUntil = millis()+700;
  } else if(config==CF_TIME){
    Serial.printf("[chronos] time %s\n", watch.getTimeDate().c_str());
  } else if(config==CF_NAV_DATA){
    refreshNavFromWatch();
    navHide=false;
    Serial.printf("[chronos] nav active=%d %s\n", (int)navActive, navTitle.c_str());
  } else if(config==CF_NAV_ICON){
    Navigation &nv = watch.getNavigation();
    if(nv.iconCRC != navIconCRC){
      navIconCRC = nv.iconCRC;
      refreshNavFromWatch();
    }
  }
}
void chronosSetupCallbacks(){
  watch.setConnectionCallback(onChronosConn);
  watch.setNotificationCallback(onChronosNotif);
  watch.setRingerCallback(onChronosRinger);
  watch.setConfigurationCallback(onChronosConfig);
}
void chronosApply(){
  if(chronosOn){
    if(!watch.isRunning()){
      chronosSetupCallbacks();
      watch.begin();
      Serial.printf("[chronos] BLE on mac=%s\n", watch.getAddress().c_str());
    }
  } else {
    chronoConn=false; ringerOn=false; notifUntil=0; navActive=false; navHide=false;
    if(watch.isRunning()) watch.stop();
    Serial.println("[chronos] BLE off");
  }
}

void serviceChronosFind(){
  if(!findUntil) return;
  if((int32_t)(millis()-findUntil)>=0){ findUntil=0; return; }
  if(((millis()/120)&1)==0) tft.fillScreen(C_YEL);
  else tft.fillScreen(C_BG);
}
