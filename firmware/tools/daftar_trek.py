#!/usr/bin/env python3
"""Daftar model -> nomor trek DFPlayer, dan cek kartu SD (opsional). Hanya membaca, tidak mengubah apa pun.

Tata letak kartu (default firmware 0.7.1, URUTAN SALIN):
  /0001.mp3 .. /0048.mp3  root: suara animasi (trek klip) + suara Chronos 0041-0048. Isi siap pakai: sd/mp3/.
                          Kartu baru diformat, salin 0001..0048 BERURUTAN dulu, baru folder /01.
  /01/001.mp3 ..          lagu untuk Pemutar MP3 di menu (maks 255)
Build -DMOCHI_DF_MP3_FOLDER: file root itu ditaruh di /MP3/0001.mp3 .. (dicocokkan dari NAMA, urutan bebas).

Sumber nomor trek = include/jpeg_clips.h hasil build (dibuat tools/embed_jpeg.py saat `pio run`).
Jika header belum ada, skrip memakai assets/meta.json (perkiraan; jalankan `pio run` untuk angka pasti).

Contoh:
  python tools/daftar_trek.py                       # tabel model, trek, nama file
  python tools/daftar_trek.py --sd /media/SD        # cek kartu
  python tools/daftar_trek.py --sd /media/SD --mp3-folder   # build -DMOCHI_DF_MP3_FOLDER
"""
import argparse, json, pathlib, re, sys

root = pathlib.Path(__file__).resolve().parents[1]

def from_header(p):
    rows = []
    # {"tema","stem",JPGF_i,JPGS_i,n,delay}  atau  {...,n,delay,role,track,x,y}
    for m in re.finditer(r'^\s*\{"([^"]+)","([^"]+)",JPGF_\d+,JPGS_\d+,([^}]*)\},\s*$', p.read_text(), re.M):
        nums = [int(x) for x in m.group(3).split(",")]
        n = nums[0]
        delay = nums[1] if len(nums) >= 4 else None
        role = {1: "goyang", 2: "tahan"}.get(nums[2], "") if len(nums) >= 4 else ""
        track = nums[3] if len(nums) >= 4 else len(rows) + 1
        rows.append((m.group(1), m.group(2), n, delay, track, role))
    return rows

def from_meta():
    meta = json.loads((root / "assets" / "meta.json").read_text())
    if meta.get("jpeg_clips"):
        roles = {"dizzy": "goyang", "heart": "tahan"}
        return [(e[0], e[1], None, None, e[3] if len(e) > 3 else 0, roles.get(e[2] if len(e) > 2 else "", ""))
                for e in meta["jpeg_clips"]]
    return [(t, s, None, None, i + 1, "") for i, (t, s) in enumerate(meta["builtins"])]

def ci_dir(base, name):
    if (base / name).is_dir(): return base / name
    for p in base.iterdir():
        if p.is_dir() and p.name.lower() == name.lower(): return p
    return None

def files(d):
    return {p.name.lower(): p for p in d.iterdir() if p.is_file()} if d else {}

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sd", help="folder root kartu SD DFPlayer yang sudah dipasang")
    ap.add_argument("--mp3-folder", action="store_true", help="build -DMOCHI_DF_MP3_FOLDER: suara di /MP3")
    a = ap.parse_args()
    a.copy_order = not a.mp3_folder

    hdr = root / "include" / "jpeg_clips.h"
    rows = from_header(hdr) if hdr.exists() else []
    src = "include/jpeg_clips.h"
    if not rows:
        rows, src = from_meta(), "assets/meta.json (perkiraan, header build belum ada)"
    prefix = "" if a.copy_order else "MP3/"
    print(f"Sumber: {src}\n")
    print(f"{'Model':>5}  {'Trek':>4}  {'File SD':<14} {'Frame':>9}  Tema/klip")
    for i, (t, s, n, d, tr, role) in enumerate(rows):
        fr = "" if n is None else (f"{n}x{d}ms" if d else f"{n}")
        extra = f"  ({role})" if role else ""
        print(f"{i + 1:>5}  {tr:>4}  /{prefix}{tr:04d}.mp3 {fr:>9}  {t}/{s}{extra}")

    seen = []
    for t in [r[0] for r in rows]:
        if seen and seen[-1] == t: continue
        if t in seen: print(f"\nPERINGATAN: tema '{t}' tidak berurutan. Goyang/tahan memakai klip di kelompok tema yang sama.")
        seen.append(t)

    if not a.sd: return 0
    sd = pathlib.Path(a.sd)
    if not sd.is_dir():
        print(f"\n{sd} bukan folder."); return 1
    anim = sd if a.copy_order else ci_dir(sd, "MP3")
    want = {f"{r[4]:04d}.mp3" for r in rows} | {f"{k:04d}.mp3" for k in range(41, 49)}  # + Chronos
    have = files(anim)
    junk = sorted(str(p.relative_to(sd)) for p in sd.rglob("*") if p.name.startswith("._") or p.name.lower() == ".ds_store")
    missing = sorted(w for w in want if w not in have)
    extra = sorted(n for n, p in have.items() if p.suffix.lower() == ".mp3" and n not in want and not n.startswith("._"))
    if a.copy_order:  # 0001-0048 di root semuanya wajar (pengisi hening menjaga urutan salin)
        extra = [n for n in extra if not (re.fullmatch(r"\d{4}\.mp3", n) and int(n[:4]) <= 48)]
    print(f"\nCek kartu {sd}:")
    print(f"  Suara animasi ({'root' if a.copy_order else '/MP3'}):")
    if anim is None: print("    folder /MP3 TIDAK ADA")
    print("    kurang :", ", ".join(missing) or "-")
    print("    lebih  :", ", ".join(extra) or "-")
    music = ci_dir(sd, "01")
    mf = sorted(n for n in files(music) if re.fullmatch(r"\d{3}.*\.mp3", n))
    print(f"  Musik /01 : {len(mf)} lagu" + ("" if mf else " (pemutar MP3 akan kosong)"))
    if mf:
        nums = sorted(int(n[:3]) for n in mf)
        gaps = [k for k in range(1, nums[-1] + 1) if k not in nums]
        if gaps: print("    nomor bolong:", ", ".join(f"{k:03d}" for k in gaps), "(lagu sesudah lubang tidak tercapai lewat 'berikutnya')")
        if nums[-1] > 255: print("    PERINGATAN: DFPlayer hanya bisa 001..255 per folder")
    if a.copy_order:
        nums = sorted(int(n[:4]) for n in have if re.fullmatch(r"\d{4}\.mp3", n))
        gaps = [k for k in range(1, 49) if k not in nums]
        if gaps: print("    nomor bolong:", ", ".join(f"{k:04d}" for k in gaps),
                       "<- urutan salin bergeser! isi dengan file hening (sd/mp3 sudah lengkap 0001-0048)")
    if junk:
        print("  SAMPAH macOS:", ", ".join(junk[:8]), "..." if len(junk) > 8 else "", "<- hapus (ikut terhitung sebagai file)")
    if a.copy_order:
        print("\n  Urutan salin tidak terlihat dari nama file. Kalau suara tertukar: format kartu, salin sd/mp3/0001..0048")
        print("  ke root berurutan (satu per satu), baru folder /01.")
    return 1 if missing or anim is None else 0

if __name__ == "__main__":
    sys.exit(main())
