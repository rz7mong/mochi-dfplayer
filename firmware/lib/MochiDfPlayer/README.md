# MochiDfPlayer

Library suara untuk build `esp32-c3-dfplayer`. Firmware GIF, menu, sentuh, SD, dan Chronos sama dengan MAX98357. Suara tidak dicampur ke `firmware/src/main.cpp`.

```bash
cd firmware
pio run -e esp32-c3-dfplayer -t upload
```

Kabel: VCC 5V, GND, RX modul <- GPIO20 lewat ~1k, TX modul -> GPIO21, speaker di SPK. Lepas MAX98357.

## SD modul (FAT32)

| Folder | Isi |
|---|---|
| 01/001.mp3 .. 011.mp3 | reaksi: raspberry, squint, love_hearts_kiss, angry_2, smirk, sleepy, yawn_tired, rainbow, pong, revs, hadouken_hit |
| 02/001.mp3 .. | ekspresi wajah, nomor = indeks bawaan + 1 |
| 06/001.mp3 .. 009.mp3 | tema: wajah, gundam, mobil, polisi, musik, neon, anime, makanan, intro |
| 03/001.mp3 | notifikasi Chronos |
| 04/001.mp3 | dering Chronos, diulang sampai ditutup |
| 05/001.mp3 .. | lagu pemutar |

Ganti tema, ekspresi, atau reaksi memutar file pasangannya. Notifikasi dan dering Chronos memutar 03 dan 04. Menu Musik memutar folder 05 (putar/jeda, berikut, sebelumnya). API: `POST /api/music` body `{"action":"play"|"next"|"prev"|"toggle"|"stop"}`.
