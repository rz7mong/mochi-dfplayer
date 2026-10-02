#!/usr/bin/env python3
"""Impor klip JPEG 240x240 siap pakai (tanpa batas 6 frame) ke assets/builtin/jpeg/<theme>/<stem>.{mjpeg,json}.

Sumber: header C berisi array frame JPEG (`NAME_jpg_frame_N[] PROGMEM` + `NAME_frames[]`), folder *.jpg / *.png, atau GIF.
  python3 tools/import_jpeg_clip.py mochi full1 path/full1.h
  python3 tools/import_jpeg_clip.py mochi video17 folder_frame/
  python3 tools/import_jpeg_clip.py gundam kokpit kokpit.gif --quality 75
Frame boleh lebih kecil dari 240x240 (mis. 160x80); firmware menggambarnya di tengah layar dengan latar hitam (x/y di json).
GIF: tiap frame diulang round(durasi/delay) slot supaya tempo asli terjaga; default delay = durasi frame terpendek.

Hemat flash tanpa mengubah tempo:
  * frame yang nyaris identik dengan frame sebelumnya (<= --hold piksel berbeda > 24 level) tidak disimpan ulang;
    urutan `seq` menunjuk frame lama, jadi jumlah frame dan durasi klip tetap sama.
  * tiap frame unik dioptimasi lossless dengan `jpegtran -optimize` (jika ada), tetap baseline untuk TJpg_Decoder.
  * --quality Q (opsional) meng-encode ulang frame unik (lossy). Default: tidak.
Hasil: <stem>.mjpeg (frame unik disambung) + <stem>.json {"delay","sizes","seq"}. Dibaca tools/embed_jpeg.py."""
import argparse, io, json, pathlib, re, shutil, subprocess, sys
from PIL import Image, ImageChops, ImageFilter

root = pathlib.Path(__file__).resolve().parents[1]
ap = argparse.ArgumentParser()
ap.add_argument("theme"); ap.add_argument("stem"); ap.add_argument("src")
ap.add_argument("--delay", type=int, default=0, help="ms per frame (default 100; GIF: durasi frame terpendek)")
ap.add_argument("--hold", type=int, default=57, help="maks piksel berbeda agar frame dianggap sama (57 = 0,1%% dari 240x240)")
ap.add_argument("--quality", type=int, default=0, help="encode ulang (lossy); GIF default 80")
ap.add_argument("--step", type=int, default=1, help="ambil 1 dari tiap N frame, tempo tetap (delay x N)")
ap.add_argument("--smooth", type=float, default=0, help="blur Gaussian (px) sebelum encode, hilangkan dither GIF")
ap.add_argument("--resize", default="", help="WxH, mis. 240x120: Lanczos + unsharp ringan (perlu --quality)")
ap.add_argument("--sharpen", type=int, default=60, help="persen unsharp setelah --resize (0 = tanpa)")
ap.add_argument("--crop", default="", help="WxH potong tengah setelah resize, mis. 240x240")
ap.add_argument("--subsampling", type=int, default=2, help="0 = 4:4:4 (warna tajam), 2 = 4:2:0")
ap.add_argument("--outdir", default="", help="default assets/builtin/jpeg")
a = ap.parse_args()

src = pathlib.Path(a.src)
images = None  # GIF: daftar (PIL image, jumlah slot)
if src.is_dir():
    frames = [p.read_bytes() for p in sorted(src.glob("*.jpg"))]
    pngs = sorted(src.glob("*.png"))
    if not frames and pngs:  # folder PNG (mis. hasil ffmpeg dari MP4): selalu di-encode
        images = [(Image.open(p).convert("RGB"), 1) for p in pngs]
        frames = [None] * len(images)
        if not a.quality: a.quality = 80
elif src.suffix.lower() == ".gif":
    g = Image.open(src); durs, imgs = [], []
    for i in range(g.n_frames):
        g.seek(i); imgs.append(g.convert("RGB")); durs.append(g.info.get("duration", 100) or 100)
    if not a.delay: a.delay = max(40, min(durs))
    if not a.quality: a.quality = 80
    images = [(im, max(1, round(d / a.delay))) for im, d in zip(imgs, durs)]
    frames = [None] * len(images)
else:
    t = src.read_text()
    arrs = {int(m.group(1)): bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(2)))
            for m in re.finditer(r"jpg_frame_(\d+)\[\]\s*PROGMEM\s*=\s*\{(.*?)\};", t, re.S)}
    order = re.search(r"\w*frames\[\][^{]*\{(.*?)\};", t, re.S)
    idx = [int(x) for x in re.findall(r"jpg_frame_(\d+)", order.group(1))] if order else sorted(arrs)
    frames = [arrs[i] for i in idx]
if not a.delay: a.delay = 100
if (a.resize or a.crop) and not a.quality: a.quality = 80
if a.step > 1:  # GIF: slot diulang; header/folder: delay dikali N
    if images: images = [(im, n * a.step) for im, n in images[::a.step]]
    else: a.delay *= a.step
    frames = frames[::a.step]
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
last = size = None
for fi, raw in enumerate(frames):
    im, slots = images[fi] if images else (Image.open(io.BytesIO(raw)).convert("RGB"), 1)
    if a.resize:
        w, h = map(int, a.resize.lower().split("x"))
        im = im.resize((w, h), Image.LANCZOS)
        if a.sharpen: im = im.filter(ImageFilter.UnsharpMask(radius=1.0, percent=a.sharpen, threshold=2))
    if a.crop:
        w, h = map(int, a.crop.lower().split("x")); l, t = (im.width - w) // 2, (im.height - h) // 2
        im = im.crop((l, t, l + w, t + h))
    if size is None: size = im.size
    if im.size != size or im.size[0] > 240 or im.size[1] > 240: sys.exit(f"frame {fi} berukuran {im.size}")
    if last is not None and changed(im, last) <= a.hold * im.size[0] * im.size[1] // 57600:
        seq += [len(uniq) - 1] * slots; continue
    last = im
    out = raw
    if a.quality:
        if a.smooth: im = im.filter(ImageFilter.GaussianBlur(a.smooth))
        b = io.BytesIO(); im.save(b, "JPEG", quality=a.quality, optimize=True, subsampling=a.subsampling); out = b.getvalue()
    if jt:
        opt = subprocess.run([jt, "-optimize", "-copy", "none"], input=out, capture_output=True, check=True).stdout
        if len(opt) < len(out): out = opt
    if sof(out) != 0xC0: sys.exit("frame bukan JPEG baseline")
    Image.open(io.BytesIO(out)).load()
    uniq.append(out); sizes.append(len(out)); seq += [len(uniq) - 1] * slots

dst = (pathlib.Path(a.outdir) if a.outdir else root / "assets" / "builtin" / "jpeg") / a.theme
dst.mkdir(parents=True, exist_ok=True)
(dst / f"{a.stem}.mjpeg").write_bytes(b"".join(uniq))
(dst / f"{a.stem}.json").write_text(json.dumps({"delay": a.delay, "w": size[0], "h": size[1],
    "x": (240 - size[0]) // 2, "y": (240 - size[1]) // 2, "sizes": sizes, "seq": seq}, separators=(",", ":")) + "\n")
print(f"{a.theme}/{a.stem}: {len(seq)} slot x {a.delay} ms, {len(uniq)} unik, {size[0]}x{size[1]}, {sum(sizes)} byte")
