#!/usr/bin/env python3
# Vessel - Copyright (C) 2026 BroBordd
# SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
"""
Makes the <song>.vsd files the music window reads (notes + drum hits per song), offline, from the lossless
originals. The game then just looks the data up at the playback position instead of guessing from the live
audio: no lag, no overtone ghosts, no drum bleed into the notes, and it follows scrubbing.

    python tools/gen_music_data.py ~/flacs -o music/            # every .flac/.wav/.ogg in the folder
    python tools/gen_music_data.py ~/flacs/divine_tale.flac -o music/ --cache /tmp/vsd_stems

the output name comes from the input name, so divine_tale.flac -> music/divine_tale.vsd (next to divine_tale.ogg).

pipeline per song:
  1. separate into drums / bass / other (+ vocals folded into other) with Demucs     [--sep demucs]
     (no Demucs? --sep hpss splits harmonic / percussive with librosa, rougher; --sep none uses the mix)
  2. drums stem  -> kick / snare / hi-hat hits (per-band onset detection, with cross-suppression)
  3. bass stem   -> notes (basic-pitch, neural polyphonic transcription, with real note onsets / offsets)
     other stem  -> notes (same)
  4. merge, normalise loudness, write the .vsd (see the format at the top of game/analyze.c)

install (PC, python 3.9-3.12):
    pip install numpy scipy librosa soundfile
    pip install --no-deps basic-pitch ; pip install onnxruntime resampy mir_eval pretty_midi scikit-learn
    pip install demucs                      # optional but this is what removes the pollution (needs torch)
(basic-pitch's own pip metadata asks for tensorflow, which is why --no-deps: the bundled .onnx model is used)

the slow step is Demucs, so --cache keeps the stems and re-runs with other thresholds take seconds.
"""
import argparse
import os
import struct
import sys
import tempfile

import numpy as np

SR = 44100
AUDIO_EXT = (".flac", ".wav", ".ogg", ".mp3", ".m4a")


# ----------------------------------------------------------------------------------------------- audio io
def load_audio(path):
    import soundfile as sf
    y, sr = sf.read(path, dtype="float32", always_2d=True)       # [T, ch]
    y = y.T
    if y.shape[0] == 1:
        y = np.repeat(y, 2, axis=0)
    y = y[:2]
    if sr != SR:
        from scipy.signal import resample_poly
        from math import gcd
        g = gcd(sr, SR)
        y = resample_poly(y, SR // g, sr // g, axis=1).astype(np.float32)
    return np.ascontiguousarray(y)


def mono(y):
    return y.mean(axis=0)


def save_stem(path, y):
    import soundfile as sf
    sf.write(path, y.T, SR, subtype="PCM_24")


# ----------------------------------------------------------------------------------------- separation
_demucs = {}


def separate_demucs(y, device):
    import torch
    from demucs.apply import apply_model
    from demucs.pretrained import get_model
    if "m" not in _demucs:
        _demucs["m"] = get_model("htdemucs").eval()
    model = _demucs["m"]
    dev = device or ("cuda" if torch.cuda.is_available() else "cpu")
    wav = torch.from_numpy(y)
    ref = wav.mean(0)
    w = (wav - ref.mean()) / (ref.std() + 1e-8)
    with torch.no_grad():
        out = apply_model(model, w[None], device=dev, split=True, overlap=0.25, progress=True)[0]
    out = (out * ref.std() + ref.mean()).cpu().numpy()
    st = dict(zip(model.sources, out))
    other = st["other"] + st.get("vocals", 0)                    # a sung lead is a lead: the piano shows it too
    return {"drums": st["drums"], "bass": st["bass"], "other": other}


def separate_hpss(y):
    import librosa
    m = mono(y)
    h, p = librosa.effects.hpss(m, margin=(1.0, 3.0))
    return {"drums": np.stack([p, p]), "other": np.stack([h, h])}


def get_stems(path, y, sep, cache, device):
    name = os.path.splitext(os.path.basename(path))[0]
    cdir = os.path.join(cache, name, sep) if cache else None
    if cdir and os.path.isdir(cdir):
        import soundfile as sf
        stems = {}
        for f in os.listdir(cdir):
            if f.endswith(".flac"):
                a, _ = sf.read(os.path.join(cdir, f), dtype="float32", always_2d=True)
                stems[f[:-5]] = a.T
        if stems:
            print("  stems from cache")
            return stems
    if sep == "demucs":
        stems = separate_demucs(y, device)
    elif sep == "hpss":
        stems = separate_hpss(y)
    else:
        stems = {"drums": y, "other": y}
    if cdir:
        os.makedirs(cdir, exist_ok=True)
        for k, v in stems.items():
            save_stem(os.path.join(cdir, k + ".flac"), v)
    return stems


# ------------------------------------------------------------------------------------------------ drums
# band floors: a flux peak below this is wobble, not a hit (same idea and units as the live analyzer)
KICK_FLOOR, SNARE_FLOOR, HAT_FLOOR = 0.03, 1.6, 1.4


def _flux(mag, sr, n_fft, lo, hi):
    """positive spectral flux of log-compressed magnitudes summed over lo..hi Hz -> [frames]"""
    k0, k1 = max(1, int(lo * n_fft / sr)), int(hi * n_fft / sr) + 1
    c = np.log1p(300.0 * mag[k0:k1] / (n_fft / 4.0))
    d = np.diff(c, axis=1, prepend=c[:, :1])
    return np.maximum(d, 0).sum(axis=0)


def _pick(env, fps, gap_s, floor, rel, echo=0.5, min_rel=0.0):
    from scipy.ndimage import median_filter
    from scipy.signal import find_peaks
    if len(env) < 8:
        return np.array([], int), np.array([])
    ref = np.percentile(env, 97) + 1e-9
    w = max(3, int(0.5 * fps) | 1)
    thr = median_filter(env, size=w, mode="nearest") + rel * ref
    pk, _ = find_peaks(env, height=np.maximum(thr, floor), distance=max(1, int(gap_s * fps)))
    # a drum rings: the wobble 40-150 ms after a hit is not a new hit unless it is about as big as that hit
    a, b = max(1, int(0.02 * fps)), max(2, int(0.15 * fps))
    top = np.percentile(env, 99.5) + 1e-9                             # the big hits of this song
    pk = pk[env[pk] >= min_rel * top]                                # leaks from other drums are much quieter than real hits
    pk = np.array([i for i in pk if i - b < 0 or env[i] >= echo * env[max(0, i - b):max(1, i - a)].max()], int)
    strength = np.clip(env[pk] / max(top, floor * 1.5), 0.2, 1.0) if len(pk) else np.array([])
    return pk, strength


def _kick_env(y, hop):
    """kick = a rise in the 40-110 Hz range, measured in the time domain. (in the log-compressed spectrum every
    snare / hat click leaks a little into the lows and looks like a kick; in the waveform it does not)"""
    from scipy.signal import butter, sosfiltfilt, hilbert
    sos = butter(6, [40.0, 110.0], "bp", fs=SR, output="sos")      # steep: a snare body at ~190 Hz must not leak in
    e = np.abs(hilbert(sosfiltfilt(sos, y))).astype(np.float32)
    k = 441                                                         # 10 ms moving average
    e = np.convolve(e, np.ones(k, np.float32) / k, mode="same")
    e = np.sqrt(e)[::hop]
    return np.maximum(np.diff(e, prepend=e[:1]), 0)


def detect_drums(drum_stem, hop=256, shift_ms=0.0, rel=0.15):
    import librosa
    y = mono(drum_stem)
    sr = SR
    S2 = np.abs(librosa.stft(y, n_fft=2048, hop_length=hop))     # kick / snare body: needs the bass resolution
    S1 = np.abs(librosa.stft(y, n_fft=1024, hop_length=hop))     # snare noise / hats: needs the time resolution
    kick = _kick_env(y, hop)[: S2.shape[1]]
    body = _flux(S2, sr, 2048, 190, 540)
    noise = _flux(S1, sr, 1024, 2000, 6000)
    snare = 0.6 * body + 0.4 * noise
    hat = _flux(S1, sr, 1024, 7000, 15000)
    fps = sr / hop
    hits = {}
    for name, env, gap, floor, min_rel in (("kick", kick, 0.060, KICK_FLOOR, 0.55), ("snare", snare, 0.080, SNARE_FLOOR, 0.30),
                                           ("hat", hat, 0.035, HAT_FLOOR, 0.20)):
        pk, st = _pick(env, fps, gap, floor, rel, min_rel=min_rel)
        if name == "snare" and len(pk):                              # a snare has a body (190-540 Hz) AND a noise rattle (2-6 kHz):
            bt, nt = np.percentile(body, 99.5) + 1e-9, np.percentile(noise, 99.5) + 1e-9   # a kick has only the first, a hat only the second
            ok = np.array([body[max(0, i - 2):i + 3].max() >= 0.3 * bt and noise[max(0, i - 2):i + 3].max() >= 0.25 * nt for i in pk])
            pk, st = pk[ok], st[ok]
        hits[name] = (pk / fps + shift_ms / 1000.0, st)

    def near(t, arr, tol):
        if len(arr) == 0:
            return None
        i = np.argmin(np.abs(arr - t))
        return i if abs(arr[i] - t) <= tol else None

    kt, ks = hits["kick"]
    st_, ss = hits["snare"]
    ht, hs = hits["hat"]
    keep = np.ones(len(st_), bool)                               # a kick has some mid in it: not a snare
    for i, t in enumerate(st_):
        j = near(t, kt, 0.025)
        if j is not None and ss[i] < 0.6 * ks[j]:
            keep[i] = False
    st_, ss = st_[keep], ss[keep]
    keep = np.ones(len(ht), bool)                                # a snare has a lot of highs in it: not a hat
    for i, t in enumerate(ht):
        j = near(t, st_, 0.025)
        if j is not None and hs[i] < 0.8 * ss[j]:
            keep[i] = False
    ht, hs = ht[keep], hs[keep]
    out = []
    for code, (t, s) in enumerate(((kt, ks), (st_, ss), (ht, hs))):
        out += [(float(a), code, float(b)) for a, b in zip(t, s) if a >= 0]
    out.sort()
    return out


# ------------------------------------------------------------------------------------------------ notes
def transcribe(stem, lo_hz, hi_hz, onset=0.5, frame=0.3, min_ms=80.0):
    """basic-pitch on one stem -> [(start_s, end_s, midi, amplitude 0..1)]"""
    import soundfile as sf
    from basic_pitch import ICASSP_2022_MODEL_PATH
    from basic_pitch.inference import predict
    with tempfile.TemporaryDirectory() as td:
        p = os.path.join(td, "s.wav")
        sf.write(p, mono(stem), SR)
        _, _, ev = predict(p, model_or_model_path=ICASSP_2022_MODEL_PATH, onset_threshold=onset,
                           frame_threshold=frame, minimum_note_length=min_ms,
                           minimum_frequency=lo_hz, maximum_frequency=hi_hz)
    return sorted((float(e[0]), float(e[1]), int(e[2]), float(e[3])) for e in ev)


def detect_notes(stems, onset, frame, min_ms):
    has_bass = "bass" in stems
    notes = []
    if has_bass:
        notes += [n + ("bass",) for n in transcribe(stems["bass"], 30.0, 330.0, onset, frame, max(min_ms, 100.0))]
    lo = 130.0 if has_bass else 30.0                              # with a bass stem, low notes in "other" are bleed
    notes += [n + ("other",) for n in transcribe(stems["other"], lo, 2400.0, onset, frame, min_ms)]
    # same pitch sounding twice at (almost) the same time -> keep the stronger one
    notes.sort()
    out = []
    for n in notes:
        dup = next((m for m in reversed(out[-12:]) if m[2] == n[2] and abs(m[0] - n[0]) < 0.06), None)
        if dup is None:
            out.append(n)
        elif n[3] > dup[3]:
            out[out.index(dup)] = n
    return [(a, b, p, v) for a, b, p, v, _ in sorted(out)]


# ------------------------------------------------------------------------------------------------ output
def write_vsd(path, notes, hits, duration_s):
    amps = np.array([n[3] for n in notes]) if notes else np.array([1.0])
    ref = np.percentile(amps, 95) + 1e-9                          # loudest ~5% of notes land on full velocity
    rows = []
    for s, e, p, a in notes:
        vel = int(round(255 * min(1.0, a / ref) ** 0.7))
        rows.append((int(round(s * 1000)), min(65535, max(30, int(round((e - s) * 1000)))), p, vel))
    rows.sort()
    hrows = sorted((int(round(max(0.0, t) * 1000)), c, int(round(255 * s))) for t, c, s in hits)
    with open(path, "wb") as f:
        f.write(b"VSD1" + struct.pack("<III", len(rows), len(hrows), int(duration_s * 1000)))
        for r in rows:
            f.write(struct.pack("<IHBB", *r))
        for r in hrows:
            f.write(struct.pack("<IBB", *r))
    return len(rows), len(hrows)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="an audio file, or a folder of them")
    ap.add_argument("-o", "--out", default="music", help="output folder (default: music/)")
    ap.add_argument("--sep", choices=["auto", "demucs", "hpss", "none"], default="auto")
    ap.add_argument("--cache", help="folder to keep separated stems in (re-runs skip Demucs)")
    ap.add_argument("--device", help="torch device for Demucs (cuda / cpu / mps)")
    ap.add_argument("--onset", type=float, default=0.5, help="note onset threshold, lower = more notes (0.5)")
    ap.add_argument("--frame", type=float, default=0.3, help="note sustain threshold, lower = longer notes (0.3)")
    ap.add_argument("--min-note-ms", type=float, default=80.0)
    ap.add_argument("--min-vel", type=float, default=0.12, help="drop notes quieter than this fraction (0.12)")
    ap.add_argument("--drum-sensitivity", type=float, default=0.15, help="drum threshold, LOWER = more hits (0.15)")
    ap.add_argument("--drum-shift-ms", type=float, default=0.0, help="nudge every drum hit in time")
    ap.add_argument("--force", action="store_true", help="overwrite existing .vsd files")
    a = ap.parse_args()

    if a.sep == "auto":
        try:
            import demucs  # noqa: F401
            a.sep = "demucs"
        except ImportError:
            print("demucs is not installed: using the rough harmonic/percussive split (pip install demucs for a clean one)")
            a.sep = "hpss"

    files = [a.input] if os.path.isfile(a.input) else sorted(
        os.path.join(a.input, f) for f in os.listdir(a.input) if f.lower().endswith(AUDIO_EXT))
    if not files:
        sys.exit("no audio files found")
    os.makedirs(a.out, exist_ok=True)
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        dst = os.path.join(a.out, name + ".vsd")
        if os.path.exists(dst) and not a.force:
            print(f"{name}: exists, skipping (--force to redo)")
            continue
        print(f"{name}:")
        y = load_audio(path)
        stems = get_stems(path, y, a.sep, a.cache, a.device)
        hits = detect_drums(stems["drums"], shift_ms=a.drum_shift_ms, rel=a.drum_sensitivity)
        notes = detect_notes(stems, a.onset, a.frame, a.min_note_ms)
        if a.min_vel > 0 and notes:
            ref = np.percentile([n[3] for n in notes], 95)
            notes = [n for n in notes if n[3] / ref >= a.min_vel]
        nn, nh = write_vsd(dst, notes, hits, y.shape[1] / SR)
        kinds = [sum(1 for h in hits if h[1] == c) for c in range(3)]
        print(f"  {nn} notes, {nh} drum hits (kick {kinds[0]}, snare {kinds[1]}, hat {kinds[2]}) -> {dst}")
        if not os.path.exists(os.path.splitext(dst)[0] + ".ogg"):
            print(f"  note: no {name}.ogg next to it")


if __name__ == "__main__":
    main()
