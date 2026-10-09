/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * runs the music analyzer over an .ogg on a PC and prints what it hears each half second:
 *   cc -O2 -Igame -o build/antest tools/antest.c game/analyze.c -lm && build/antest music/third_life.ogg [seconds] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyze.h"
#include "third_party/stb_vorbis.c"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    int ch, rate; short *pcm; int n = stb_vorbis_decode_filename(argv[1], &ch, &rate, &pcm);
    if (n <= 0) { printf("decode failed\n"); return 1; }
    printf("%s: %d frames, %d ch, %d Hz\n", argv[1], n, ch, rate);
    float maxsec = argc > 2 ? (float)atof(argv[2]) : 20.0f;
    static const char *NAMES[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    an_reset();
    int pos = 0, chunk = 1024, nk = 0, ns = 0, nh = 0, chunks = 0;
    static short st[2048];
    while (pos + chunk <= n && pos < maxsec * rate) {
        for (int i = 0; i < chunk; i++) { st[i*2] = pcm[(pos+i)*ch]; st[i*2+1] = pcm[(pos+i)*ch + (ch > 1)]; }
        an_feed(st, chunk); pos += chunk; chunks++;
        float k, s, h; an_drums(&k, &s, &h); nk += k > 0; ns += s > 0; nh += h > 0;
        if (chunks % 21 == 0) {
            float nt[AN_NOTES], b, m, l; an_notes(nt); an_registers(&b, &m, &l);
            printf("%5.1fs drums(k%d s%d h%d) bass %.2f mid %.2f lead %.2f  notes:", (float)pos / rate, nk, ns, nh, b, m, l);
            nk = ns = nh = 0;
            for (int i = 0; i < AN_NOTES; i++) if (nt[i] > 0) printf(" %s%d", NAMES[(AN_NOTE_LO + i) % 12], (AN_NOTE_LO + i) / 12 - 1);
            printf("\n");
        }
    }
    return 0;
}
