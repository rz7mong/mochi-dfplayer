# MochiDfPlayer

Jalur suara DFPlayer Mini (UART 9600) untuk Mochi DFPlayer 0.7.0.

Kabel: VCC 5V, GND, RX modul ← GPIO20 lewat ±1 kΩ, TX modul → GPIO21, speaker di SPK_1/SPK_2. BUSY → GPIO5 opsional (`-DMOCHI_PIN_DF_BUSY=5`).

## Kartu SD

| Path | Fungsi | Perintah |
|---|---|---|
| `/MP3/0001.mp3` … | suara animasi (`mochiDfPlayTrack`) | 0x12, menurut nama |
| root `0001.mp3` … | sama, jika `-DMOCHI_DF_COPY_ORDER` | 0x03, menurut urutan salin |
| `/01/001.mp3` … `255` | pemutar MP3 (`mochiDfPlayMusic`) | 0x0F |
| `/02/001.mp3` | notifikasi Chronos (`mochiDfPlayNotif`) | 0x0F |
| `/03/*.mp3` | dering Chronos, diulang (`mochiDfPlayRinger`) | 0x17 |

## Trek selesai dan status

- Modul mengirim pesan 0x3D sendiri saat trek habis (sering dua kali; yang ganda dibuang). `mochiDfTakeFinished()` mengembalikan `true` sekali per trek.
- Jika pin BUSY dipasang, naiknya BUSY ke HIGH juga dihitung sebagai selesai (jeda tidak dihitung).
- `mochiDfQueryState()` menanyakan status (0x42), memblok s/d ±200 ms. Dipakai firmware hanya di layar pemutar, tiap 2,5 dtk, sebagai cadangan.
- `mochiDfTakeError()` meneruskan kode error modul (5/6 = file tidak ada), dipakai untuk kembali ke lagu 001 setelah lagu terakhir.
- `mochiDfMusicCount()` menanyakan jumlah file di `/01` (0x4E).

Volume 0–30 dikirim hanya jika berubah; antarperintah selalu ada jeda ≥ 80 ms. Saat init firmware menunggu 1,2 dtk sejak nyala agar modul selesai membaca kartu.
