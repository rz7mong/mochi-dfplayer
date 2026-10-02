#!/usr/bin/env python3
"""Bake GIF clips into JPEG frames, Dasai/Pikapet style, for ESP flash.
GIF builtins (meta "builtins") tetap maks 6 frame. Klip JPEG siap pakai (meta "jpeg_clips": [[theme, stem, role], ...],
role "" / "dizzy" / "heart") dibaca dari assets/builtin/jpeg/<theme>/<stem>.{mjpeg,json} dengan semua framenya,
dibuat oleh tools/import_jpeg_clip.py. Urutan: builtins dulu, lalu jpeg_clips (nomor trek DFPlayer = urutan)."""
import io, json, pathlib
from PIL import Image
root = pathlib.Path(__file__).resolve().parents[1]
meta = json.loads((root / "assets" / "meta.json").read_text())
gif_root = root / "assets" / "builtin" / "gif"
inc = root / "include"
parts = ["#pragma once", "#include <Arduino.h>"]
rows = []
total = 0
for i, (theme, stem) in enumerate(meta["builtins"]):
    gp = gif_root / theme / f"{stem}.gif"
    im = Image.open(gp)
    frames, durs = [], []
    n = 0
    while True:
        frames.append(im.convert("RGB"))
        durs.append(im.info.get("duration", 100) or 100)
        n += 1
        try:
            im.seek(n)
        except EOFError:
            break
    if len(frames) > 6:
        step = max(1, len(frames) // 6)
        frames = frames[::step][:6]
        durs = durs[::step][:6]
    delay = max(40, sum(durs) // len(durs))
    ptrs, sizes = [], []
    for fi, fr in enumerate(frames):
        buf = io.BytesIO()
        fr.save(buf, format="JPEG", quality=62, optimize=True)
        raw = buf.getvalue()
        total += len(raw)
        name = f"JPG_{i}_{fi}"
        hexes = ",".join(f"0x{b:02x}" for b in raw)
        parts.append(f"static const uint8_t {name}[] PROGMEM = {{{hexes}}};")
        ptrs.append(name)
        sizes.append(str(len(raw)))
    parts.append(f"static const uint8_t* const JPGF_{i}[] = {{{','.join(ptrs)}}};")
    parts.append(f"static const uint16_t JPGS_{i}[] = {{{','.join(sizes)}}};")
    rows.append(f'  {{"{theme}","{stem}",JPGF_{i},JPGS_{i},{len(frames)},{delay},0}},')
    print(f"{theme}/{stem}: {len(frames)} frames")
jpeg_root = root / "assets" / "builtin" / "jpeg"
ROLES = {"": 0, "dizzy": 1, "heart": 2}
for k, ent in enumerate(meta.get("jpeg_clips", [])):
    theme, stem = ent[0], ent[1]
    role = ROLES[ent[2] if len(ent) > 2 else ""]
    i = len(rows)
    info = json.loads((jpeg_root / theme / f"{stem}.json").read_text())
    blob = (jpeg_root / theme / f"{stem}.mjpeg").read_bytes()
    if sum(info["sizes"]) != len(blob):
        raise SystemExit(f"{theme}/{stem}: ukuran mjpeg tidak cocok dengan json")
    names, off = [], 0
    for fi, sz in enumerate(info["sizes"]):
        raw = blob[off:off + sz]; off += sz
        if raw[:2] != b"\xff\xd8" or raw[-2:] != b"\xff\xd9":
            raise SystemExit(f"{theme}/{stem}: frame {fi} bukan JPEG utuh")
        total += sz
        name = f"JPG_{i}_{fi}"
        parts.append(f"static const uint8_t {name}[] PROGMEM = {{{','.join(f'0x{b:02x}' for b in raw)}}};")
        names.append(name)
    seq = info["seq"]
    parts.append(f"static const uint8_t* const JPGF_{i}[] = {{{','.join(names[j] for j in seq)}}};")
    parts.append(f"static const uint16_t JPGS_{i}[] = {{{','.join(str(info['sizes'][j]) for j in seq)}}};")
    rows.append(f'  {{"{theme}","{stem}",JPGF_{i},JPGS_{i},{len(seq)},{info.get("delay", 100)},{role}}},')
    print(f"{theme}/{stem}: {len(seq)} frames ({len(names)} unik, {len(blob)} B)")
parts.append("struct JpegClip { const char* theme; const char* stem; const uint8_t* const* frames; const uint16_t* sizes; uint16_t n; uint16_t delay; uint8_t role; };")
parts.append("enum : uint8_t { JPEG_ROLE_NONE = 0, JPEG_ROLE_DIZZY = 1, JPEG_ROLE_HEART = 2 };")
parts.append("static const JpegClip JPEG_CLIPS[] = {")
parts.extend(rows)
parts.append("};")
parts.append(f"static const int JPEG_CLIP_COUNT = {len(rows)};")
parts.append(f'static const char* const JPEG_BOOT_THEME = "{meta.get("boot_theme", "")}";')
(inc / "jpeg_clips.h").write_text("\n".join(parts) + "\n")
print("jpeg bytes", total)
