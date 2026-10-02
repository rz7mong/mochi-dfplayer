#!/usr/bin/env python3
"""Impor klip JPEG 240x240 siap pakai (tanpa batas 6 frame) ke assets/builtin/jpeg/<theme>/<stem>.{mjpeg,json}.

Sumber: header C berisi array frame JPEG (`NAME_jpg_frame_N[] PROGMEM` + `NAME_frames[]`) atau folder *.jpg.
  python3 tools/import_jpeg_clip.py mochi full1 path/full1.h
  python3 tools/import_jpeg_clip.py mochi video17 folder_frame/

Hemat flash tanpa mengubah tempo:
  * frame yang nyaris identik dengan frame sebelumnya (<= --hold piksel berbeda > 24 level) tidak disimpan ulang;
    urutan `seq` menunjuk frame lama, jadi jumlah frame dan durasi klip tetap sama.
  * tiap frame unik dioptimasi lossless dengan `jpegtran -optimize` (jika ada), tetap baseline untuk TJpg_Decoder.
  * --quality Q (opsional) meng-encode ulang frame unik (lossy). Default: tidak.
Hasil: <stem>.mjpeg (frame unik disambung) + <stem>.json {"delay","sizes","seq"}. Dibaca tools/embed_jpeg.py."""
import argparse, io, json, pathlib, re, shutil, subprocess, sys
from PIL import Image, ImageChops

root = pathlib.Path(__file__).resolve().parents[1]
ap = argparse.ArgumentParser()
ap.add_argument("theme"); ap.add_argument("stem"); ap.add_argument("src")
ap.add_argument("--delay", type=int, default=100, help="ms per frame (info; main.cpp memakai FRAME_MS)")
ap.add_argument("--hold", type=int, default=57, help="maks piksel berbeda agar frame dianggap sama (57 = 0,1%% dari 240x240)")
ap.add_argument("--quality", type=int, default=0)
a = ap.parse_args()

src = pathlib.Path(a.src)
if src.is_dir():
    frames = [p.read_bytes() for p in sorted(src.glob("*.jpg"))]
else:
    t = src.read_text()
    arrs = {int(m.group(1)): bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(2)))
            for m in re.finditer(r"_jpg_frame_(\d+)\[\]\s*PROGMEM\s*=\s*\{(.*?)\};", t, re.S)}
    order = re.search(r"\w+_frames\[\][^{]*\{(.*?)\};", t, re.S)
    idx = [int(x) for x in re.findall(r"_jpg_frame_(\d+)", order.group(1))] if order else sorted(arrs)
    frames = [arrs[i] for i in idx]
if not frames: sys.exit("tidak ada frame")

def sof(b):
    i = 2
    while i + 4 <= len(b):
        m, L = b[i + 1], (b[i + 2] << 8) | b[i + 3]
        if 0xC0 <= m <= 0xCF and m not in (0xC4, 0xC8, 0xCC): return m
        i += 2 + L
    return None

def changed(x, y):
    d = ImageChops.difference(x, y).split()
    m = d[0]
    for c in d[1:]: m = ImageChops.lighter(m, c)
    return sum(m.point(lambda v: 255 if v > 24 else 0).histogram()[255:])

jt = shutil.which("jpegtran")
uniq, sizes, seq = [], [], []
last = None
for raw in frames:
    im = Image.open(io.BytesIO(raw)).convert("RGB")
    if im.size != (240, 240): sys.exit(f"frame {len(seq)} berukuran {im.size}, harus 240x240")
    if last is not None and changed(im, last) <= a.hold:
        seq.append(len(uniq) - 1); continue
    last = im
    out = raw
    if a.quality:
        b = io.BytesIO(); im.save(b, "JPEG", quality=a.quality, optimize=True, subsampling=2); out = b.getvalue()
    if jt:
        opt = subprocess.run([jt, "-optimize", "-copy", "none"], input=out, capture_output=True, check=True).stdout
        if len(opt) < len(out): out = opt
    if sof(out) != 0xC0: sys.exit("frame bukan JPEG baseline")
    Image.open(io.BytesIO(out)).load()
    uniq.append(out); sizes.append(len(out)); seq.append(len(uniq) - 1)

dst = root / "assets" / "builtin" / "jpeg" / a.theme
dst.mkdir(parents=True, exist_ok=True)
(dst / f"{a.stem}.mjpeg").write_bytes(b"".join(uniq))
(dst / f"{a.stem}.json").write_text(json.dumps({"delay": a.delay, "sizes": sizes, "seq": seq}, separators=(",", ":")) + "\n")
print(f"{a.theme}/{a.stem}: {len(seq)} frame, {len(uniq)} unik, {sum(len(f) for f in frames)} -> {sum(sizes)} byte")
