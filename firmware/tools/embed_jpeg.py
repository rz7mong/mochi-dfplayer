#!/usr/bin/env python3
"""Bake GIF clips into JPEG frames, Dasai/Pikapet style, for ESP flash."""
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
    if len(frames) > 4:
        step = max(1, len(frames) // 4)
        frames = frames[::step][:4]
        durs = durs[::step][:4]
    delay = max(40, sum(durs) // len(durs))
    ptrs, sizes = [], []
    for fi, fr in enumerate(frames):
        buf = io.BytesIO()
        fr.save(buf, format="JPEG", quality=32, optimize=True)
        raw = buf.getvalue()
        total += len(raw)
        name = f"JPG_{i}_{fi}"
        hexes = ",".join(f"0x{b:02x}" for b in raw)
        parts.append(f"static const uint8_t {name}[] PROGMEM = {{{hexes}}};")
        ptrs.append(name)
        sizes.append(str(len(raw)))
    parts.append(f"static const uint8_t* const JPGF_{i}[] = {{{','.join(ptrs)}}};")
    parts.append(f"static const uint16_t JPGS_{i}[] = {{{','.join(sizes)}}};")
    rows.append(f'  {{"{theme}","{stem}",JPGF_{i},JPGS_{i},{len(frames)},{delay}}},')
    print(f"{theme}/{stem}: {len(frames)} frames")
parts.append("struct JpegClip { const char* theme; const char* stem; const uint8_t* const* frames; const uint16_t* sizes; uint8_t n; uint16_t delay; };")
parts.append("static const JpegClip JPEG_CLIPS[] = {")
parts.extend(rows)
parts.append("};")
parts.append(f"static const int JPEG_CLIP_COUNT = {len(rows)};")
(inc / "jpeg_clips.h").write_text("\n".join(parts) + "\n")
print("jpeg bytes", total)
