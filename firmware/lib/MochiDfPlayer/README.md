# MochiDfPlayer

Jalur suara DFPlayer Mini (UART 9600) untuk build `esp32-c3-dfplayer`.

Kabel: VCC 5V, GND, RX modul ← GPIO20 lewat ±1 kΩ, TX modul → GPIO21, speaker di SPK_1/SPK_2.

## Yang dipakai firmware sekarang

`src/main.cpp` hanya memanggil `mochiDfInit()`, `mochiDfSetVolume()`, `mochiDfStop()`, dan `mochiDfPlayTrack(n)`.

- `mochiDfPlayTrack(n)` memutar trek ke-n di **root** kartu (`0001.mp3` … ), menurut **urutan salin** FAT (perintah `0x03`).
- Build dengan `-DMOCHI_DF_MP3_FOLDER` agar memutar `/MP3/000n.mp3` menurut **nama file** (perintah `0x12`).
- Volume dikirim sekali saat init (28 dari 30) dan hanya dikirim ulang jika berubah. Ada jeda ≥ 80 ms antarperintah.
- `begin()` dipanggil tanpa ACK, jadi firmware tidak tahu apakah modul benar-benar tersambung.

## Fungsi lain (belum dipakai `main.cpp`)

Sisa dari firmware MAX98357 dan disimpan untuk fitur berikutnya (menu, Chronos, pemutar musik). Struktur folder yang mereka harapkan:

| Folder | Isi |
|---|---|
| `01/001.mp3` … `011.mp3` | reaksi (`mochiDfPlayReact`) |
| `02/001.mp3` … | ekspresi wajah (`mochiDfPlayFace`) |
| `03/001.mp3` | notifikasi (`mochiDfPlayNotif`) |
| `04/001.mp3` | dering, diulang (`mochiDfPlayRinger`) |
| `05/001.mp3` … | lagu (`mochiDfMusicStart/Next/Prev/Toggle`) |
| `06/001.mp3` … `009.mp3` | tema (`mochiDfPlayTheme`) |
