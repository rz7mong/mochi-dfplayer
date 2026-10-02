#pragma once
#include <Arduino.h>

/* Jalur suara DFPlayer Mini (UART 9600). Tata letak kartu SD (FAT32):
 *   /0001.mp3 .. /0048.mp3  root: suara animasi (trek klip) + suara Chronos 0041-0048. Diputar menurut
 *                           URUTAN SALIN (perintah 0x03): salin 0001..0048 berurutan ke kartu kosong DULU.
 *                           -DMOCHI_DF_MP3_FOLDER: file di /MP3/000N.mp3, dicocokkan dari NAMA (0x12).
 *   /01/001.mp3 ..255       musik untuk pemutar MP3 di menu (perintah 0x0F), disalin SETELAH root.
 */
#define MOCHI_DF_FOLDER_MUSIC 1
#define MOCHI_DF_ERR_FILE_INDEX 5   // kode error modul: nomor file di luar jangkauan
#define MOCHI_DF_ERR_FILE_MISSING 6 // kode error modul: file tidak ditemukan

enum class DfState : uint8_t { Unknown, Stopped, Playing, Paused };

bool mochiDfInit();
bool mochiDfReady();
void mochiDfService();                 // panggil tiap loop: baca pesan modul + pin BUSY
void mochiDfVolume(int vol30);         // 0..30
int mochiDfGetVolume();
void mochiDfStop();
void mochiDfPause();
void mochiDfResume();
bool mochiDfPlayTrack(uint16_t track); // trek root (urutan salin): animasi / Chronos
bool mochiDfPlayMusic(uint16_t file);  // /01/<file>.mp3
int mochiDfMusicCount(bool refresh);   // jumlah file di /01, -1 jika modul tidak menjawab
bool mochiDfLoopTrack(uint16_t track); // trek diulang sampai stop (dering/cari/alarm Chronos)
bool mochiDfTakeFinished();            // true sekali tiap trek selesai (pesan 0x3D atau BUSY naik)
bool mochiDfTakeError(uint16_t *code); // true sekali tiap pesan error modul (mis. 6 = file tidak ada)
DfState mochiDfQueryState();           // tanya status ke modul (memblok s/d ±200 ms)
bool mochiDfBusyPin();                 // true jika pin BUSY dipasang
