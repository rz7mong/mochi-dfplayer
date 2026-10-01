#!/usr/bin/env python3
"""Embed built-in GIF+WAV into flash (PROGMEM in app partition -> include/defaults_gif.h) + jingle.h.
List/order comes from assets/meta.json "builtins": [[theme, stem], ...]; first entry should be wajah/default.
GIF: assets/builtin/gif/<theme>/<stem>.gif   WAV: assets/builtin/sfx/<theme>/<stem>.wav (16-bit PCM mono, any rate)
Missing GIF => hard error (no silent stand-ins). Missing WAV => slot plays jingle."""
import math, struct, pathlib, json, wave, sys
root = pathlib.Path(__file__).resolve().parents[1]
inc = root / "include"; inc.mkdir(exist_ok=True)
A = root / "assets" / "builtin"
meta = json.loads((root / "assets" / "meta.json").read_text())

def dump(name, raw, ctype="uint8_t"):
    out = [f"const {ctype} {name}[] PROGMEM = {{"]
    for i in range(0, len(raw), 32):
        out.append("".join(f"0x{b:02x}," for b in raw[i:i+32]))
    out.append("};")
    return "\n".join(out)

def read_wav(p):
    with wave.open(str(p), "rb") as w:
        if w.getsampwidth() != 2: sys.exit(f"{p}: need 16-bit PCM")
        ch, sr, data = w.getnchannels(), w.getframerate(), w.readframes(w.getnframes())
    if ch == 2:
        s = struct.unpack(f"<{len(data)//2}h", data)
        data = struct.pack(f"<{len(s)//2}h", *[(s[i]+s[i+1])//2 for i in range(0, len(s), 2)])
    return data, sr

g = ["#pragma once", "#include <Arduino.h>",
     "struct DefaultGif { const char *theme; const char *stem; const uint8_t *data; int len; const int16_t *pcm; int pcmSamples; int pcmRate; };"]
rows = []; tot_gif = tot_pcm = 0
for i, (theme, stem) in enumerate(meta["builtins"]):
    gp = A / "gif" / theme / f"{stem}.gif"
    if not gp.exists() or gp.read_bytes()[:3] != b"GIF": sys.exit(f"missing built-in GIF {gp}")
    raw = gp.read_bytes(); tot_gif += len(raw)
    g.append(dump(f"BG_{i}", raw)); pcm, ns, sr = "nullptr", 0, 0
    wp = A / "sfx" / theme / f"{stem}.wav"
    if wp.exists():
        data, sr = read_wav(wp); ns = len(data)//2; tot_pcm += len(data)
        g.append(f"alignas(4) " + dump(f"BS_{i}", data).replace("const uint8_t", "const uint8_t", 1))
        pcm = f"(const int16_t*)BS_{i}"
    rows.append(f'  {{"{theme}","{stem}",BG_{i},{len(raw)},{pcm},{ns},{sr}}},')
    print(f"{theme}/{stem}: gif {len(raw)} B, pcm {ns*2} B @ {sr} Hz")
g += ["static const DefaultGif DEFAULT_GIFS[] = {"] + rows + ["};",
      f"static const int DEFAULT_GIF_COUNT = {len(rows)};"]
(inc / "defaults_gif.h").write_text("\n".join(g) + "\n")
print(f"TOTAL gif {tot_gif} + pcm {tot_pcm} = {tot_gif+tot_pcm} bytes in {len(rows)} slots")
sr = 8000; notes = [(523,0.10),(659,0.10),(784,0.14),(1046,0.16)]; samples = []
for freq, leng in notes:
    nn = int(sr*leng)
    for i in range(nn):
        env = min(1.0, i/70.0)*min(1.0, (nn-i)/100.0)
        samples.append(int(10000*env*math.sin(2*math.pi*freq*i/sr)))
pcm = b"".join(struct.pack("<h", s) for s in samples)
(inc / "jingle.h").write_text("\n".join(["#pragma once", "#include <Arduino.h>", f"const int JINGLE_SR={sr};",
    dump("JINGLE_PCM", pcm), f"const int JINGLE_PCM_LEN = {len(pcm)};"]) + "\n")
print("embedded ok")
