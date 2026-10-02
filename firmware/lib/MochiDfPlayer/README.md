# MochiDfPlayer

Jalur suara DFPlayer Mini (UART 9600) untuk Mochi DFPlayer 0.8.1.

Kabel: VCC 5V, GND, RX modul ← GPIO20 lewat ±1 kΩ, TX modul → GPIO21, speaker di SPK_1/SPK_2. BUSY → GPIO5 opsional (`-DMOCHI_PIN_DF_BUSY=5`).

## Kartu SD

| Path | Fungsi | Perintah |
|---|---|---|
| root `0001.mp3` … `0048.mp3` | suara animasi + Chronos (`mochiDfPlayTrack`), **bawaan** | 0x03, menurut **urutan salin** |
| sama, diulang | dering/cari/alarm Chronos (`mochiDfLoopTrack`) | 0x08 (ulang satu trek) |
| `/MP3/0001.mp3` … | sama, jika `-DMOCHI_DF_MP3_FOLDER` (loop diputar ulang oleh pustaka saat selesai) | 0x12, menurut nama |
| `/01/001.mp3` … `255` | pemutar MP3 (`mochiDfPlayMusic`) | 0x0F |

Urutan salin dihitung global (folder ikut), jadi salin root 0001–0048 ke kartu yang baru diformat **sebelum** folder `/01`. Isi siap pakai: `sd/mp3/` di root repo. Trek yang diulang (`mochiDfLoopTrack`) tidak dilaporkan selesai; hentikan dengan `mochiDfStop()` atau trek lain.

## Trek selesai dan status

- Modul mengirim pesan 0x3D sendiri saat trek habis (sering dua kali; yang ganda dibuang). `mochiDfTakeFinished()` mengembalikan `true` sekali per trek.
- Jika pin BUSY dipasang, naiknya BUSY ke HIGH juga dihitung sebagai selesai (jeda tidak dihitung).
- `mochiDfQueryState()` menanyakan status (0x42), memblok s/d ±200 ms. Dipakai firmware hanya di layar pemutar, tiap 2,5 dtk, sebagai cadangan.
- `mochiDfTakeError()` meneruskan kode error modul (5/6 = file tidak ada), dipakai untuk kembali ke lagu 001 setelah lagu terakhir.
- `mochiDfMusicCount()` menanyakan jumlah file di `/01` (0x4E).

Volume 0–30 dikirim hanya jika berubah; antarperintah selalu ada jeda ≥ 80 ms. Saat init firmware menunggu 1,2 dtk sejak nyala agar modul selesai membaca kartu.
