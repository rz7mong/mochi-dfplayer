#!/usr/bin/env python3
"""Daftar model -> nomor trek DFPlayer, dan cek kartu SD (opsional). Hanya membaca, tidak mengubah apa pun.

Sumber kebenaran = include/jpeg_clips.h hasil build (dibuat tools/embed_jpeg.py saat `pio run`).
Jika header belum ada, jalankan `pio run` dulu, atau skrip ini memakai assets/meta.json "builtins"
(trek = posisi + 1, aturan firmware saat ini).

Contoh:
  python tools/daftar_trek.py                 # tabel model, trek, nama file mp3
  python tools/daftar_trek.py --sd /media/SD  # cek kartu: file kurang, file lebih, sampah macOS
  python tools/daftar_trek.py --sd /media/SD --mp3-folder   # untuk build -DMOCHI_DF_MP3_FOLDER (/MP3/0001.mp3)
"""
import argparse, json, pathlib, re, sys

root = pathlib.Path(__file__).resolve().parents[1]

def from_header(p):
    rows = []
    # {"tema","stem",JPGF_i,JPGS_i,n,delay}  atau  {...,n,delay,role,track,x,y} (format klip JPEG penuh)
    for m in re.finditer(r'^\s*\{"([^"]+)","([^"]+)",JPGF_\d+,JPGS_\d+,([^}]*)\},\s*$', p.read_text(), re.M):
        nums = [int(x) for x in m.group(3).split(",")]
        n = nums[0]
        # Format lama: firmware memakai tempo tetap FRAME_MS di main.cpp, kolom delay diabaikan.
        delay = nums[1] if len(nums) >= 4 else None
        track = nums[3] if len(nums) >= 4 else len(rows) + 1
        rows.append((m.group(1), m.group(2), n, delay, track))
    return rows

def from_meta():
    meta = json.loads((root / "assets" / "meta.json").read_text())
    return [(t, s, None, None, i + 1) for i, (t, s) in enumerate(meta["builtins"])]

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sd", help="folder root kartu SD DFPlayer yang sudah dipasang")
    ap.add_argument("--mp3-folder", action="store_true", help="file ada di /MP3/ (build -DMOCHI_DF_MP3_FOLDER)")
    a = ap.parse_args()

    hdr = root / "include" / "jpeg_clips.h"
    rows = from_header(hdr) if hdr.exists() else []
    src = "include/jpeg_clips.h"
    if not rows:
        rows, src = from_meta(), "assets/meta.json (header build belum ada)"
    print(f"Sumber: {src}\n")
    print(f"{'Model':>5}  {'Trek':>4}  {'File SD':<10} {'Frame':>5}  Tema/klip")
    for i, (t, s, n, d, tr) in enumerate(rows):
        fr = "" if n is None else (f"{n}x{d}ms" if d else f"{n}")
        print(f"{i + 1:>5}  {tr:>4}  {tr:04d}.mp3   {fr:>8}  {t}/{s}")

    themes, seen = [r[0] for r in rows], []
    for t in themes:
        if seen and seen[-1] == t: continue
        if t in seen: print(f"\nPERINGATAN: tema '{t}' tidak berurutan. Goyang/tahan memakai klip di kelompok tema yang sama.")
        seen.append(t)

    if not a.sd: return 0
    sd = pathlib.Path(a.sd)
    base = sd / "MP3" if a.mp3_folder else sd
    if a.mp3_folder and not base.is_dir():
        alt = [p for p in sd.iterdir() if p.is_dir() and p.name.lower() == "mp3"]
        base = alt[0] if alt else base
    if not base.is_dir():
        print(f"\nFolder {base} tidak ada."); return 1
    want = {f"{r[4]:04d}.mp3" for r in rows}
    have = {p.name.lower(): p for p in base.iterdir() if p.is_file()}
    junk = [n for n in have if n.startswith("._") or n in (".ds_store",)]
    missing = sorted(w for w in want if w not in have)
    extra = sorted(n for n, p in have.items() if p.suffix.lower() in (".mp3", ".wav") and n not in want and n not in junk)
    print(f"\nCek {base}:")
    print("  kurang :", ", ".join(missing) or "-")
    print("  lebih  :", ", ".join(extra) or "-")
    if junk:
        print("  SAMPAH :", ", ".join(junk), "<- hapus (file ._ dari macOS ikut terhitung sebagai trek)")
    if not a.mp3_folder:
        print("\n  Ingat: tanpa -DMOCHI_DF_MP3_FOLDER, DFPlayer memutar file ke-N menurut URUTAN SALIN, bukan nama.")
        print("  Format kartu, lalu salin 0001.mp3, 0002.mp3, ... berurutan (satu per satu, atau pakai fatsort).")
    return 1 if missing else 0

if __name__ == "__main__":
    sys.exit(main())
