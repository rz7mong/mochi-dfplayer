#pragma once
#include <Arduino.h>

bool mochiDfInit();
void mochiDfStop();
void mochiDfService();
void mochiDfSetVolume(int vol21, bool on);
bool mochiDfPlayReact(int reactIndex);
bool mochiDfPlayFace(int faceIndex);
bool mochiDfPlayTheme(const char *theme);
bool mochiDfPlayGif(const char *gifPath);
bool mochiDfPlayNotif();
void mochiDfPlayRinger(bool on);
bool mochiDfMusicStart();
bool mochiDfMusicNext();
bool mochiDfMusicPrev();
void mochiDfMusicToggle();
void mochiDfMusicStop();
bool mochiDfMusicPlaying();
