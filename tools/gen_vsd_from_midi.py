#!/usr/bin/env python3
# Vessel - Copyright (C) 2026 BroBordd
# SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
"""
Makes <song>.vsd (the music window's data, see the format at the top of game/analyze.c) straight from a MIDI file.
No audio analysis, no guessing: every note and drum hit lands exactly where the MIDI puts it.

    python tools/gen_vsd_from_midi.py ~/midis -o music/                  # every .mid in the folder
    python tools/gen_vsd_from_midi.py ~/midis/Lazy_Day.mid -o music/     # -> music/lazy_day.vsd

the output name is the input name lowercased (Lazy_Day.mid -> lazy_day.vsd), next to lazy_day.ogg.

  - tempo changes are followed (times are real seconds, not ticks)
  - channel 10 (index 9) = drums: mapped to the three drums the window knows (--drum-map shows the table)
  - every other channel = notes (pitch, start, length, velocity)
  - if the .ogg is rendered with leading silence / encoder delay, use --offset-ms to slide everything (+ = later)

needs only:  pip install mido
"""
import argparse
import os
import struct
import sys

import mido

# GM drum note -> game drum code (0 kick, 1 snare, 2 hat). low toms count as the low drum, snares / claps / rim as the
# snare, everything bright (hats, cymbals, high toms, percussion) as the hat. anything not listed -> hat.
KICK, SNARE, HAT = 0, 1, 2
DRUM_MAP = {
    35: KICK, 36: KICK,
    41: KICK, 43: KICK, 45: KICK,
    37: SNARE, 38: SNARE, 39: SNARE, 40: SNARE,
    42: HAT, 44: HAT, 46: HAT,
    47: SNARE, 48: SNARE, 50: SNARE,
}


def read_midi(path, offset_ms, drum_map):
    mid = mido.MidiFile(path)
    t = 0.0
    open_notes = {}                                 # (channel, note) -> (start_s, velocity)
    notes, hits = [], []
    for msg in mid:                                 # merged, delta times already in seconds (tempo map applied)
        t += msg.time
        if msg.type not in ("note_on", "note_off"):
            continue
        on = msg.type == "note_on" and msg.velocity > 0
        if msg.channel == 9:
            if on:
                hits.append((t, drum_map.get(msg.note, HAT), msg.velocity))
            continue
        key = (msg.channel, msg.note)
        if on:
            if key in open_notes:                   # same key struck again before it was released: close the old one
                s, v = open_notes.pop(key)
                notes.append((s, t, msg.note, v))
            open_notes[key] = (t, msg.velocity)
        elif key in open_notes:
            s, v = open_notes.pop(key)
            notes.append((s, t, msg.note, v))
    for (ch, n), (s, v) in open_notes.items():      # never released: ring to the end
        notes.append((s, t, n, v))
    off = offset_ms / 1000.0
    notes = sorted((max(0.0, s + off), max(0.0, e + off), p, v) for s, e, p, v in notes)
    hits.sort()
    # two drums of the same kind inside 20 ms (a clap + a snare on the same beat) are one hit: keep the louder
    out = []
    for h in hits:
        h = (max(0.0, h[0] + off), h[1], h[2])
        prev = next((o for o in reversed(out[-6:]) if o[1] == h[1] and h[0] - o[0] < 0.020), None)
        if prev is None:
            out.append(h)
        elif h[2] > prev[2]:
            out[out.index(prev)] = h
    return notes, out, mid.length + off


def write_vsd(path, notes, hits, duration_s):
    rows = []
    for s, e, p, v in notes:
        rows.append((int(round(s * 1000)), min(65535, max(30, int(round((e - s) * 1000)))), p, min(255, round(v * 255 / 127))))
    hrows = [(int(round(t * 1000)), c, min(255, round(v * 255 / 127))) for t, c, v in hits]
    with open(path, "wb") as f:
        f.write(b"VSD1" + struct.pack("<III", len(rows), len(hrows), int(duration_s * 1000)))
        for r in rows:
            f.write(struct.pack("<IHBB", *r))
        for r in hrows:
            f.write(struct.pack("<IBB", *r))
    return len(rows), len(hrows)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="a .mid file, or a folder of them")
    ap.add_argument("-o", "--out", default="music", help="output folder (default: music/)")
    ap.add_argument("--offset-ms", type=float, default=0.0, help="shift everything in time (+ = later), to match the .ogg")
    ap.add_argument("--drum-map", action="store_true", help="print the drum table and exit")
    ap.add_argument("--force", action="store_true", help="overwrite existing .vsd files")
    a = ap.parse_args()
    if a.drum_map:
        names = ("kick", "snare", "hat")
        for k, v in sorted(DRUM_MAP.items()):
            print(f"  GM {k:3d} -> {names[v]}")
        print("  anything else -> hat")
        return
    files = [a.input] if os.path.isfile(a.input) else sorted(
        os.path.join(a.input, f) for f in os.listdir(a.input) if f.lower().endswith((".mid", ".midi")))
    if not files:
        sys.exit("no .mid files found")
    os.makedirs(a.out, exist_ok=True)
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0].lower().replace(" ", "_")
        dst = os.path.join(a.out, name + ".vsd")
        if os.path.exists(dst) and not a.force:
            print(f"{name}: exists, skipping (--force to redo)")
            continue
        notes, hits, dur = read_midi(path, a.offset_ms, DRUM_MAP)
        nn, nh = write_vsd(dst, notes, hits, dur)
        kinds = [sum(1 for h in hits if h[1] == c) for c in range(3)]
        print(f"{name}: {nn} notes, {nh} drum hits (kick {kinds[0]}, snare {kinds[1]}, hat {kinds[2]}), {dur:.1f}s -> {dst}")
        if not os.path.exists(os.path.splitext(dst)[0] + ".ogg"):
            print(f"  note: no {name}.ogg next to it")


if __name__ == "__main__":
    main()
