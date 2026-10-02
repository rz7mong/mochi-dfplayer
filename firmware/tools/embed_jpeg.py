#!/usr/bin/env python3
"""Bake clips into JPEG frames, Dasai/Pikapet style, for ESP flash -> include/jpeg_clips.h.

* GIF builtins (meta "builtins" [[theme, stem], ...]): maks 6 frame, trek DFPlayer = posisi + 1.
  meta "skip_builtins" ["tema/stem", ...]: tidak ditanam; trek tetap dicadangkan supaya nomor lain tidak bergeser.
* Klip JPEG penuh (meta "jpeg_clips" [[theme, stem, role, track?], ...], role "" / "dizzy" / "heart"):
  dibaca dari assets/builtin/jpeg/<theme>/<stem>.{mjpeg,json}, dibuat tools/import_jpeg_clip.py, semua framenya.
  - Jika theme/stem sama dengan builtin, klip penuh MENGGANTIKAN builtin itu di posisi dan nomor trek yang sama.
  - Klip baru masuk tepat setelah kelompok temanya (tema tetap berurutan untuk ganti model / goyang / tahan),
    tema baru di akhir. Trek klip baru: lanjut dari nomor terbesar, urut sesuai daftar jpeg_clips.
Urutan model = urutan JPEG_CLIPS; nomor trek disimpan per klip, jadi trek lama tidak bergeser."""
import io, json, pathlib
from PIL import Image
root = pathlib.Path(__file__).resolve().parents[1]
meta = json.loads((root / "assets" / "meta.json").read_text())
gif_root = root / "assets" / "builtin" / "gif"
jpeg_root = root / "assets" / "builtin" / "jpeg"
inc = root / "include"
ROLES = {"": 0, "dizzy": 1, "heart": 2}
FRAME_MS = 100  # GIF builtins: tempo pemutar lama

def gif_clip(theme, stem):
    im = Image.open(gif_root / theme / f"{stem}.gif")
    frames = []
    for n in range(getattr(im, "n_frames", 1)):
        im.seek(n); frames.append(im.convert("RGB"))
    if len(frames) > 6:
        step = max(1, len(frames) // 6)
        frames = frames[::step][:6]
    out = []
    for fr in frames:
        buf = io.BytesIO(); fr.save(buf, format="JPEG", quality=62, optimize=True); out.append(buf.getvalue())
    return {"frames": out, "seq": list(range(len(out))), "delay": FRAME_MS, "x": 0, "y": 0}

def jpeg_clip(theme, stem):
    info = json.loads((jpeg_root / theme / f"{stem}.json").read_text())
    blob = (jpeg_root / theme / f"{stem}.mjpeg").read_bytes()
    if sum(info["sizes"]) != len(blob): raise SystemExit(f"{theme}/{stem}: ukuran mjpeg tidak cocok dengan json")
    out, off = [], 0
    for fi, sz in enumerate(info["sizes"]):
        raw = blob[off:off + sz]; off += sz
        if raw[:2] != b"\xff\xd8" or raw[-2:] != b"\xff\xd9": raise SystemExit(f"{theme}/{stem}: frame {fi} bukan JPEG utuh")
        out.append(raw)
    return {"frames": out, "seq": info["seq"], "delay": info.get("delay", FRAME_MS), "x": info.get("x", 0), "y": info.get("y", 0)}

jents = [(e[0], e[1], ROLES[e[2] if len(e) > 2 else ""], e[3] if len(e) > 3 else 0) for e in meta.get("jpeg_clips", [])]
jmap = {(t, s): (r, tr) for t, s, r, tr in jents}
builtins = [tuple(b) for b in meta["builtins"]]
skip = set(meta.get("skip_builtins", []))  # "tema/stem": builtin tidak ditanam, nomor treknya dibiarkan kosong
order = []  # (theme, stem, kind, role, track)
for i, (t, s) in enumerate(builtins):
    if f"{t}/{s}" in skip: pass
    elif (t, s) in jmap: order.append((t, s, "jpeg", jmap[(t, s)][0], i + 1))
    else: order.append((t, s, "gif", 0, i + 1))
    if i + 1 == len(builtins) or builtins[i + 1][0] != t:  # akhir kelompok tema: sisipkan klip baru tema ini
        order += [(jt, js, "jpeg", r, tr) for jt, js, r, tr in jents if jt == t and (jt, js) not in builtins]
order += [(jt, js, "jpeg", r, tr) for jt, js, r, tr in jents if (jt, js) not in builtins and jt not in {b[0] for b in builtins}]
nxt = max(len(builtins), max((tr for *_, tr in jents), default=0)) + 1
tracks = {}
for jt, js, r, tr in jents:
    if (jt, js) in builtins: continue
    if not tr: tr, nxt = nxt, nxt + 1
    tracks[(jt, js)] = tr

parts = ["#pragma once", "#include <Arduino.h>"]
rows, total = [], 0
for i, (theme, stem, kind, role, track) in enumerate(order):
    track = tracks.get((theme, stem), track)
    c = gif_clip(theme, stem) if kind == "gif" else jpeg_clip(theme, stem)
    names = []
    for fi, raw in enumerate(c["frames"]):
        total += len(raw); name = f"JPG_{i}_{fi}"
        parts.append(f"static const uint8_t {name}[] PROGMEM = {{{','.join(f'0x{b:02x}' for b in raw)}}};")
        names.append(name)
    parts.append(f"static const uint8_t* const JPGF_{i}[] = {{{','.join(names[j] for j in c['seq'])}}};")
    parts.append(f"static const uint16_t JPGS_{i}[] = {{{','.join(str(len(c['frames'][j])) for j in c['seq'])}}};")
    rows.append(f'  {{"{theme}","{stem}",JPGF_{i},JPGS_{i},{len(c["seq"])},{c["delay"]},{role},{track},{c["x"]},{c["y"]}}},')
    print(f"model {i + 1:2d} trek {track:04d}.mp3 {theme}/{stem}: {kind} {len(c['seq'])} frame x {c['delay']} ms, "
          f"{len(names)} unik, {sum(map(len, c['frames']))} B")
parts.append("struct JpegClip { const char* theme; const char* stem; const uint8_t* const* frames; const uint16_t* sizes;"
             " uint16_t n; uint16_t delay; uint8_t role; uint8_t track; int16_t x; int16_t y; };")
parts.append("enum : uint8_t { JPEG_ROLE_NONE = 0, JPEG_ROLE_DIZZY = 1, JPEG_ROLE_HEART = 2 };")
parts.append("static const JpegClip JPEG_CLIPS[] = {")
parts.extend(rows)
parts.append("};")
parts.append(f"static const int JPEG_CLIP_COUNT = {len(rows)};")
parts.append(f'static const char* const JPEG_BOOT_THEME = "{meta.get("boot_theme", "")}";')
(inc / "jpeg_clips.h").write_text("\n".join(parts) + "\n")
print("jpeg bytes", total)
