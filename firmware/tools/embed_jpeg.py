#!/usr/bin/env python3
"""Bake clips into JPEG frames, Dasai/Pikapet style, for ESP flash -> include/jpeg_clips.h.

* GIF builtins (meta "builtins" [[theme, stem], ...]): maks 6 frame, trek DFPlayer = posisi + 1.
  meta "skip_builtins" ["tema/stem", ...]: tidak ditanam; trek tetap dicadangkan supaya nomor lain tidak bergeser.
* Klip JPEG penuh (meta "jpeg_clips" [[theme, stem, role, track?], ...], role "" / "dizzy" / "heart"):
  dibaca dari assets/builtin/jpeg/<theme>/<stem>.{mjpeg,json}, dibuat tools/import_jpeg_clip.py, semua framenya.
  - Jika theme/stem sama dengan builtin, klip penuh MENGGANTIKAN builtin itu di posisi dan nomor trek yang sama.
  - Klip baru masuk tepat setelah kelompok temanya (tema tetap berurutan untuk ganti model / goyang / tahan),
    tema baru di akhir. Trek klip baru: lanjut dari nomor terbesar, urut sesuai daftar jpeg_clips.
meta "theme_order" (opsional): urutan tema, tema lain di belakang.
Urutan model = urutan JPEG_CLIPS; nomor trek disimpan per klip, jadi trek lama tidak bergeser.

Batas: <= 65535 frame per klip, <= 65535 B per frame, trek 1..3000. Anggaran flash: env MOCHI_JPEG_BUDGET."""
import io, json, os, pathlib
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
theme_order = meta.get("theme_order", [])  # opsional: urutan tema model, mis. ["wajah", "mobil", "gundam"]
if theme_order:  # sort stabil: urutan klip di dalam tema tetap
    order.sort(key=lambda o: theme_order.index(o[0]) if o[0] in theme_order else len(theme_order))
CHRONOS_TRK = range(41, 49)  # 0041-0048 = suara Chronos (TRK_* di MochiRzmong.h), jangan dipakai klip
nxt = max(len(builtins), max((tr for *_, tr in jents), default=0), CHRONOS_TRK[-1]) + 1
tracks = {}
for jt, js, r, tr in jents:
    if (jt, js) in builtins: continue
    if not tr:
        while nxt in CHRONOS_TRK: nxt += 1
        tr, nxt = nxt, nxt + 1
    if tr in CHRONOS_TRK:
        raise SystemExit(f"{jt}/{js}: trek {tr:04d} milik Chronos (0041-0048). Kosongkan nomor agar otomatis mulai 0049.")
    tracks[(jt, js)] = tr

# --- Muat semua klip dulu, lalu (jika perlu) muatkan ke anggaran flash, baru tulis header. ---
clips = []
for i, (theme, stem, kind, role, track) in enumerate(order):
    track = tracks.get((theme, stem), track)
    c = gif_clip(theme, stem) if kind == "gif" else jpeg_clip(theme, stem)
    c.update(theme=theme, stem=stem, kind=kind, role=role, track=track)
    clips.append(c)

def used_bytes():
    return sum(sum(len(c["frames"][j]) for j in set(c["seq"])) for c in clips)

# Anggaran byte JPEG (custom_jpeg_budget di platformio.ini, lewat env MOCHI_JPEG_BUDGET). 0 = tanpa batas.
# Jika total melebihi anggaran, frame yang NYARIS SAMA dengan frame yang tampil sebelumnya (<= ambang piksel
# berbeda > 24 level) diganti frame sebelumnya, sama seperti --hold di import_jpeg_clip.py. Ambang dinaikkan
# bertahap dan seragam untuk semua klip sampai muat. Tempo tidak berubah; aset sumber tidak diubah.
budget = int(os.environ.get("MOCHI_JPEG_BUDGET", "0") or 0)
before = used_bytes()
if budget and before > budget:
    from PIL import ImageChops
    gray = {}
    def g(ci, j):
        k = (ci, j)
        if k not in gray: gray[k] = Image.open(io.BytesIO(clips[ci]["frames"][j])).convert("L")
        return gray[k]
    def ndiff(ci, a, b):
        h = ImageChops.difference(g(ci, a), g(ci, b)).histogram()
        return sum(h[25:])
    orig = [list(c["seq"]) for c in clips]
    cache = {}
    def fit(th):
        for ci, c in enumerate(clips):
            seq, last = [], None
            for j in orig[ci]:
                if last is not None and j != last:
                    k = (ci, min(j, last), max(j, last))
                    if k not in cache: cache[k] = ndiff(ci, j, last)
                    if cache[k] <= th: seq.append(last); continue
                seq.append(j); last = j
            seq[0] = orig[ci][0]  # frame pertama selalu asli (titik ulang klip)
            c["seq"] = seq
        return used_bytes()
    px = 240 * 240
    chosen = None
    for pct in [round(0.1 * k, 1) for k in range(1, 31)] + [3.5, 4, 5, 6, 8, 10]:
        if fit(int(px * pct / 100)) <= budget: chosen = pct; break
    if chosen is None:
        raise SystemExit(f"jpeg {before} B tidak muat di anggaran {budget} B walau ambang 10%. "
                         "Kurangi klip, atau naikkan custom_jpeg_budget (lalu cek ukuran app).")
    print(f"ANGGARAN: {before} B > {budget} B -> frame nyaris sama (<= {chosen}% piksel) dipakai ulang, jadi {used_bytes()} B")

parts = ["#pragma once", "#include <Arduino.h>"]
rows, total = [], 0
for i, c in enumerate(clips):
    theme, stem, kind, role, track = c["theme"], c["stem"], c["kind"], c["role"], c["track"]
    seq = c["seq"]
    if not seq: raise SystemExit(f"{theme}/{stem}: tidak ada frame")
    if len(seq) > 65535: raise SystemExit(f"{theme}/{stem}: {len(seq)} frame, maks 65535")
        if not 1 <= track <= 3000: raise SystemExit(f"{theme}/{stem}: trek {track} di luar 1..3000 (batas DFPlayer folder MP3)")
    if c["delay"] > 65535: raise SystemExit(f"{theme}/{stem}: delay terlalu besar")
    used = sorted(set(seq)); names = {}
    for j in used:
        raw = c["frames"][j]
        if len(raw) > 65535: raise SystemExit(f"{theme}/{stem}: frame {j} {len(raw)} B, maks 65535 B per frame")
        total += len(raw); name = f"JPG_{i}_{j}"
        parts.append(f"static const uint8_t {name}[] PROGMEM = {{{','.join(f'0x{b:02x}' for b in raw)}}};")
        names[j] = name
    parts.append(f"static const uint8_t* const JPGF_{i}[] = {{{','.join(names[j] for j in seq)}}};")
    parts.append(f"static const uint16_t JPGS_{i}[] = {{{','.join(str(len(c['frames'][j])) for j in seq)}}};")
    rows.append(f'  {{"{theme}","{stem}",JPGF_{i},JPGS_{i},{len(seq)},{c["delay"]},{role},{track},{c["x"]},{c["y"]}}},')
    print(f"model {i + 1:2d} trek {track:04d}.mp3 {theme}/{stem}: {kind} {len(seq)} frame x {c['delay']} ms, "
          f"{len(used)} unik, {sum(len(c['frames'][j]) for j in used)} B")
parts.append("struct JpegClip { const char* theme; const char* stem; const uint8_t* const* frames; const uint16_t* sizes;"
             " uint16_t n; uint16_t delay; uint8_t role; uint16_t track; int16_t x; int16_t y; };")
parts.append("enum : uint8_t { JPEG_ROLE_NONE = 0, JPEG_ROLE_DIZZY = 1, JPEG_ROLE_HEART = 2 };")
parts.append("static const JpegClip JPEG_CLIPS[] = {")
parts.extend(rows)
parts.append("};")
parts.append(f"static const int JPEG_CLIP_COUNT = {len(rows)};")
parts.append(f'static const char* const JPEG_BOOT_THEME = "{meta.get("boot_theme", "")}";')
out = "\n".join(parts) + "\n"
hp = inc / "jpeg_clips.h"
if not hp.exists() or hp.read_text() != out:  # tulis hanya jika berubah, supaya build ulang tidak selalu kompilasi penuh
    hp.write_text(out)
print("jpeg bytes", total, f"(anggaran {budget})" if budget else "")
