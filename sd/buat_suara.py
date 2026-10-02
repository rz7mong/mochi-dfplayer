#!/usr/bin/env python3
"""Membuat ulang sd/mp3/0001.mp3 .. 0048.mp3 untuk kartu DFPlayer Mochi (urutan salin).

  0019-0023, 0026-0031, 0034, 0038, 0040  suara 14 animasi (disintesis, lihat KLIP di bawah)
  0041-0048                               suara Chronos (bip sintetis)
  nomor lain                              MP3 hening 0,5 s (pengisi supaya urutan salin = nomor)

Semua suara dibuat dari nol dengan numpy (tanpa sampel luar), lisensi MIT seperti repo ini.

Pilihan --pack mochi-themes.zip: suara 14 animasi diganti potongan WAV asli dari paket tema rzmong
(github.com/rz7mong/mochi-rzmong rilis assets-v1). Paket itu TIDAK mencantumkan lisensi; pakai hanya untuk
kartu Anda sendiri bila Anda tidak yakin punya hak menyebarkannya.

Perlu: python3, numpy, ffmpeg (libmp3lame). Jalankan dari folder repo:
    python3 sd/buat_suara.py                  # semua sintetis (isi repo)
    python3 sd/buat_suara.py --pack mochi-themes.zip
"""
import argparse, io, os, pathlib, subprocess, tempfile, wave, zipfile
import numpy as np

R = 44100
rng = np.random.default_rng(7)


def t_(d): return np.arange(int(R * d)) / R
def sil(d): return np.zeros(int(R * d))


def env_ad(n, att=0.005, rel=0.02):
    e = np.ones(n)
    a = max(1, int(R * att)); r = max(1, min(n, int(R * rel)))
    e[:a] = np.linspace(0, 1, a)
    e[-r:] *= np.linspace(1, 0, r)
    return e


def tone(f, d, amp=0.6, decay=6.0, harm=(1, 0.3, 0.1), att=0.005):
    t = t_(d)
    w = sum(a * np.sin(2 * np.pi * f * (i + 1) * t) for i, a in enumerate(harm))
    return amp * w * np.exp(-decay * t) * env_ad(len(t), att, 0.01)


def sweep(f0, f1, d, amp=0.5, shape='sin', curve=1.0):
    """Nada meluncur f0 -> f1 (bentuk: sin, tri, saw, sq)."""
    t = t_(d); x = (t / d) ** curve
    f = f0 + (f1 - f0) * x
    ph = 2 * np.pi * np.cumsum(f) / R
    return amp * osc(ph, shape) * env_ad(len(t))


def osc(ph, shape):
    if shape == 'tri': return 2 / np.pi * np.arcsin(np.sin(ph))
    if shape == 'saw': return 2 * ((ph / (2 * np.pi)) % 1) - 1
    if shape == 'sq': return np.tanh(4 * np.sin(ph))
    return np.sin(ph)


def fm(fc, d, lfo, depth, amp=0.5, shape='sin'):
    """Nada dengan vibrato (frekuensi fc +- depth, laju lfo Hz)."""
    t = t_(d)
    f = fc + depth * np.sin(2 * np.pi * lfo * t)
    ph = 2 * np.pi * np.cumsum(f) / R
    return amp * osc(ph, shape) * env_ad(len(t), 0.01, 0.05)


def band_noise(d, lo, hi, amp=0.5, tilt=0.0):
    """Derau pita lo..hi Hz (filter di domain frekuensi)."""
    n = int(R * d)
    X = np.fft.rfft(rng.standard_normal(n))
    fr = np.fft.rfftfreq(n, 1 / R)
    m = ((fr >= lo) & (fr <= hi)).astype(float)
    if tilt: m *= (np.maximum(fr, 1) / max(lo, 1)) ** tilt
    x = np.fft.irfft(X * m, n)
    x /= np.abs(x).max() + 1e-9
    return amp * x


def lowpass(x, fc):
    X = np.fft.rfft(x); fr = np.fft.rfftfreq(len(x), 1 / R)
    return np.fft.irfft(X / np.sqrt(1 + (fr / fc) ** 4), len(x))


def mix(total, *parts):
    """parts: (detik_mulai, sinyal)."""
    out = sil(total)
    for st, x in parts:
        i = int(R * st); j = min(len(out), i + len(x))
        out[i:j] += x[:j - i]
    return out


def click(amp=0.8):
    t = t_(0.012)
    return amp * band_noise(0.012, 1500, 9000, 1) * np.exp(-400 * t)


def thump(f=60, amp=0.8):
    t = t_(0.18)
    return amp * np.sin(2 * np.pi * (f + 60 * np.exp(-30 * t)) * t) * np.exp(-18 * t)


def syllable(f, d=0.085, amp=0.5):
    """Suku kata tawa kecil 'hi' (vokal sederhana: harmonik dengan formant ~ 2,3 kHz)."""
    t = t_(d)
    f0 = f * (1 + 0.08 * np.exp(-20 * t))
    ph = 2 * np.pi * np.cumsum(f0) / R
    w = sum(np.sin(k * ph) * (1.0 / k) * (2.2 if 2000 < k * f < 3200 else 1) for k in range(1, 9))
    return amp * w * np.sin(np.pi * t / d) ** 1.5


def servo(d, f=180, amp=0.35):
    """Dengung motor servo mecha."""
    t = t_(d)
    w = osc(2 * np.pi * np.cumsum(f + 25 * np.sin(2 * np.pi * 9 * t)) / R, 'saw')
    return amp * lowpass(w, 1400) * env_ad(len(t), 0.04, 0.08)


def blip(f, d=0.07, amp=0.45): return tone(f, d, amp, decay=15, harm=(1, 0.5), att=0.002)


# ---------- suara 14 animasi (nomor = trek klip di firmware/assets/meta.json) ----------
def s_senyum_kedip():   # wajah senyum + kedip (boot)
    giggle = np.concatenate([syllable(f) for f in (820, 760, 880, 700)] + [sil(0.0)])
    kedip = mix(0.5, (0, tone(2637, 0.18, 0.35, decay=18)), (0.09, tone(3136, 0.25, 0.3, decay=14)))
    return mix(8.0, (0.1, giggle), (2.6, kedip), (4.8, syllable(900, 0.11, 0.45)), (5.0, syllable(780, 0.13, 0.4)),
               (6.4, kedip * 0.8))


def s_pusing():         # pusing: goyang menurun + per "boing"
    wob = fm(520, 1.6, 6.0, 140, 0.45, 'tri') * np.linspace(1, 0.5, int(R * 1.6))
    t = t_(1.6); wob *= 1 - 0.4 * t / 1.6
    glide = sweep(700, 220, 1.6, 0.25, 'sin')
    boing = fm(330, 0.7, 14, 90, 0.4) * np.exp(-4 * t_(0.7))
    return mix(4.4, (0.1, wob + glide), (1.9, boing))


def s_cinta():          # cinta: arpeggio hati + detak
    arp = [tone(f, 0.35, 0.4, decay=7, harm=(1, 0.2)) for f in (1047, 1319, 1568, 2093)]
    spark = [tone(f, 0.12, 0.18, decay=25) for f in (3136, 3520, 3951)]
    return mix(2.5, (0, thump(70)), (0.22, thump(60, 0.6)), (0.5, arp[0]), (0.62, arp[1]), (0.74, arp[2]),
               (0.86, arp[3]), (1.1, spark[0]), (1.18, spark[1]), (1.26, spark[2]))


def s_sorot():          # sorot: mata melirik kiri-kanan (desis) + blip
    def whoosh(up=True):
        n = band_noise(0.35, 300, 5000, 0.5, tilt=0.5 if up else -0.5)
        return n * np.sin(np.pi * t_(0.35) / 0.35) ** 2
    return mix(6.0, (0.2, whoosh(True)), (0.6, blip(1400)), (1.6, whoosh(False)), (2.0, blip(1100)),
               (3.2, whoosh(True)), (3.6, blip(1600)), (3.75, blip(2000)))


def s_sirine():         # sirine polisi naik-turun
    one = np.concatenate([sweep(650, 1350, 0.9, 0.5, 'tri', 0.7), sweep(1350, 650, 0.9, 0.5, 'tri', 0.7)])
    yelp = one + 0.3 * np.roll(one, int(R * 0.004))
    return mix(3.8, (0, yelp), (1.8, yelp))


def s_cinta_pipi():     # cinta_pipi: malu "uwu" + lonceng kecil
    t = t_(0.7)
    uwu = fm(600, 0.7, 5, 40, 0.35) * np.sin(np.pi * t / 0.7)
    uwu2 = sweep(560, 760, 0.45, 0.3) * np.sin(np.pi * t_(0.45) / 0.45)
    bells = [tone(f, 0.3, 0.2, decay=10) for f in (2349, 2794, 3520)]
    return mix(3.0, (0.05, uwu), (0.8, uwu2), (1.3, bells[0]), (1.4, bells[1]), (1.5, bells[2]))


def engine(d, f0, f1, amp=0.5, curve=1.0):
    t = t_(d); f = f0 + (f1 - f0) * (t / d) ** curve
    ph = 2 * np.pi * np.cumsum(f) / R
    w = osc(ph, 'saw') + 0.5 * osc(ph * 0.5, 'sq') + 0.25 * np.sin(ph * 2)
    w *= 1 + 0.25 * np.sin(ph * 0.25)                 # denyut silinder
    w = lowpass(w, 900) + 0.15 * band_noise(d, 200, 2500, 1)
    return amp * w / (np.abs(w).max() + 1e-9) * env_ad(len(t), 0.05, 0.15)


def s_lampu_sorot():    # mobil: klik lampu + desis sorot + mesin idle
    beam = band_noise(1.0, 400, 8000, 0.45, tilt=0.3) * np.exp(-3 * t_(1.0)) * env_ad(int(R * 1.0), 0.06, 0.1)
    return mix(6.7, (0.1, click()), (0.25, click(0.6)), (0.3, beam), (1.2, engine(3.6, 38, 44, 0.45)))


def s_speedometer():    # mobil: gas -> pindah gigi -> gas
    return mix(4.2, (0.0, engine(1.5, 45, 170, 0.5, 0.8)), (1.45, engine(0.25, 120, 90, 0.35)),
               (1.65, engine(1.6, 90, 210, 0.55, 0.8)), (3.2, engine(0.6, 200, 60, 0.3)))


def s_helm_hujan():     # gundam: hujan di helm + servo
    n = 3.0
    rain = lowpass(band_noise(n, 800, 9000, 0.25), 6000) * env_ad(int(R * n), 0.3, 0.4)
    drops = [(st, tone(rng.uniform(1800, 4000), 0.04, 0.25, decay=60)) for st in rng.uniform(0, n - 0.1, 40)]
    return mix(n, (0, rain), (0.4, servo(0.5, 160)), *drops)


def s_helm_siaga():     # gundam: helm siaga, bip peringatan
    return mix(1.6, (0, servo(0.35, 200)), (0.4, blip(1760)), (0.6, blip(1760)), (0.8, tone(2349, 0.25, 0.4, 9)))


def s_isyarat():        # gundam: isyarat radio (morse) + statis
    st = band_noise(2.6, 1000, 6000, 0.08)
    parts = [(0, st)]; x = 0.2
    for dur in (0.06, 0.06, 0.18, 0.06, 0.18, 0.18):
        parts.append((x, tone(1200, dur, 0.4, decay=1, harm=(1,)) * env_ad(int(R * dur), 0.003, 0.01))); x += dur + 0.06
    parts.append((x + 0.2, sweep(900, 1500, 0.15, 0.3)))
    return mix(2.6, *parts)


def s_kokpit():         # gundam: dengung kokpit + blip komputer
    t = t_(3.0)
    hum = (0.3 * np.sin(2 * np.pi * 60 * t) + 0.15 * np.sin(2 * np.pi * 120 * t) + 0.08 * np.sin(2 * np.pi * 180 * t))
    hum = hum * env_ad(len(t), 0.4, 0.5) + lowpass(band_noise(3.0, 50, 600, 0.06), 500)
    return mix(3.0, (0, hum), (0.5, blip(1568)), (0.62, blip(2093)), (1.4, blip(1319)), (2.1, blip(1760)),
               (2.2, blip(2349)))


def s_kokpit_2():       # gundam: sistem menyala (sapuan naik) + dengung
    up = sweep(80, 900, 1.2, 0.35, 'saw', 2.0)
    up = lowpass(up, 2500)
    t = t_(1.6)
    hum = (0.25 * np.sin(2 * np.pi * 90 * t) + 0.1 * np.sin(2 * np.pi * 180 * t)) * env_ad(len(t), 0.05, 0.6)
    return mix(3.0, (0, up), (1.15, tone(1760, 0.3, 0.4, 8)), (1.2, hum))


def s_pilot():          # gundam: pilot, servo + bunyi kunci mekanis + bip
    return mix(2.6, (0, servo(0.5, 150)), (0.55, thump(55, 0.7)), (0.56, click(0.7)), (0.8, servo(0.35, 230)),
               (1.25, blip(1319)), (1.4, blip(1760)))


KLIP = {  # trek: (nama klip, fungsi sintetis, [WAV paket tema rzmong], panjang total detik)
    19: ("wajah/senyum_kedip", s_senyum_kedip, ["wajah/happy", 0.6, "wajah/squint", 1.6, "wajah/smile", 0.9, "wajah/squint"], 8.0),
    20: ("wajah/pusing", s_pusing, ["wajah/confused_2"], 4.4),
    21: ("wajah/sorot", s_sorot, ["wajah/look_left", 0.5, "wajah/look_right", 0.5, "wajah/look_down", 0.4, "wajah/distracted"], 6.0),
    22: ("wajah/sirine", s_sirine, ["polisi/police", 0.2, "polisi/police"], 3.8),
    23: ("wajah/cinta", s_cinta, ["wajah/love_hearts_kiss"], 2.5),
    26: ("gundam/helm_hujan", s_helm_hujan, ["gundam/helm_hujan"], 3.0),
    27: ("gundam/helm_siaga", s_helm_siaga, ["gundam/helm_siaga"], 1.6),
    28: ("gundam/isyarat", s_isyarat, ["gundam/isyarat"], 2.6),
    29: ("gundam/kokpit", s_kokpit, ["gundam/kokpit"], 3.0),
    30: ("gundam/kokpit_2", s_kokpit_2, ["gundam/kokpit_2"], 3.0),
    31: ("gundam/pilot", s_pilot, ["gundam/pilot"], 2.6),
    34: ("mobil/lampu_sorot", s_lampu_sorot, ["mobil/headlights", 0.1, "mobil/car", 0.0, "mobil/car"], 6.7),
    38: ("mobil/speedometer", s_speedometer, ["mobil/accel", 0.0, "mobil/speed_3", 0.0, "mobil/revs"], 4.2),
    40: ("wajah/cinta_pipi", s_cinta_pipi, ["wajah/embarrassed", 0.3, "wajah/uwu"], 3.0),
}


# ---------- suara Chronos 0041-0048 ----------
def sq(f, d, amp=0.5):
    t = t_(d)
    return amp * np.tanh(4 * np.sin(2 * np.pi * f * t)) * env_ad(len(t), 0.003, 0.008)


def trill(d, f1=880, f2=1175, rate=20):
    t = t_(d)
    f = np.where((np.floor(t * rate) % 2) == 0, f1, f2)
    ph = 2 * np.pi * np.cumsum(f) / R
    return 0.5 * (np.sin(ph) + 0.3 * np.sin(2 * ph)) * env_ad(len(t), 0.01, 0.02)


C = np.concatenate
CHRONOS = {
    41: ("notifikasi", C([tone(1319, 0.16, decay=10), tone(1760, 0.35, decay=7), sil(0.1)])),
    42: ("navigasi_instruksi", C([tone(1047, 0.09, decay=12), sil(0.04), tone(1319, 0.09, decay=12), sil(0.04), tone(1568, 0.2, decay=9), sil(0.1)])),
    43: ("panggilan", C([trill(0.9), sil(0.2), trill(0.9), sil(1.0)])),
    44: ("cari_perangkat", C([C([sq(2093, 0.12, 0.7), sil(0.06), sq(2637, 0.12, 0.7), sil(0.12)]) for _ in range(3)])),
    45: ("alarm", C([C([sq(1000, 0.1, 0.55), sil(0.08)]) for _ in range(4)] + [sil(0.5)])),
    46: ("tersambung", C([tone(784, 0.12, decay=10), tone(1175, 0.3, decay=8), sil(0.1)])),
    47: ("terputus", C([tone(1175, 0.12, decay=10), tone(784, 0.3, decay=8), sil(0.1)])),
    48: ("navigasi_selesai", C([tone(1047, 0.1, decay=10), tone(1319, 0.1, decay=10), tone(1568, 0.1, decay=10), tone(2093, 0.45, decay=5), sil(0.1)])),
}


def read_wav(zf, name):
    for p in (f"sfx/{name}.wav", f"{name}.wav"):
        if p in zf.namelist():
            with wave.open(io.BytesIO(zf.read(p))) as w:
                sr, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
                x = np.frombuffer(w.readframes(n), '<i2').astype(float) / 32768
            if ch > 1: x = x.reshape(-1, ch).mean(1)
            return np.interp(np.arange(int(len(x) * R / sr)) * sr / R, np.arange(len(x)), x)
    raise FileNotFoundError(name)


def from_pack(zf, seq, total):
    """seq: nama WAV diselingi jeda (detik). Hasil diberi hening sampai total detik."""
    parts = [read_wav(zf, s) if isinstance(s, str) else sil(s) for s in seq]
    x = np.concatenate(parts)
    return np.concatenate([x, sil(max(0.3, total - len(x) / R))])


def enc(x, path, peak, tmp):
    if np.abs(x).max() > 0: x = x / np.abs(x).max() * peak
    pcm = (np.clip(x, -1, 1) * 32767).astype('<i2')
    with wave.open(tmp, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes(pcm.tobytes())
    subprocess.check_call(['ffmpeg', '-loglevel', 'error', '-y', '-i', tmp, '-codec:a', 'libmp3lame', '-b:a', '128k',
                           '-ar', str(R), '-ac', '1', '-map_metadata', '-1', '-id3v2_version', '0', '-write_xing', '0', path])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--pack', help='mochi-themes.zip (rz7mong/mochi-rzmong assets-v1): pakai WAV aslinya untuk 14 animasi')
    ap.add_argument('--out', default=str(pathlib.Path(__file__).resolve().parent / 'mp3'))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    tmp = os.path.join(tempfile.gettempdir(), 'mochi_suara.wav')
    zf = zipfile.ZipFile(a.pack) if a.pack else None
    for n in range(1, 49):
        p = f'{a.out}/{n:04d}.mp3'
        if n in KLIP:
            name, fn, seq, total = KLIP[n]
            x = from_pack(zf, seq, total) if zf else fn()
            enc(x, p, 0.85, tmp); print(f'{n:04d} {name:22s} {len(x) / R:4.1f} s {"paket" if zf else "sintetis"}')
        elif n in CHRONOS:
            name, x = CHRONOS[n]
            enc(x, p, 0.95 if n == 44 else 0.8, tmp); print(f'{n:04d} chronos/{name:14s} {len(x) / R:4.1f} s')
        else:
            enc(sil(0.5), p, 0, tmp)
    print('selesai:', a.out)


if __name__ == '__main__':
    main()
