/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * reads a .vsd the way the game does and prints what the music window would show, every half second:
 *   cc -O2 -Igame -o build/vsdtest tools/vsdtest.c game/analyze.c -lm && build/vsdtest music/third_life.vsd [seconds] [start] */
#include <stdio.h>
#include <stdlib.h>
#include "analyze.h"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    if (!an_load_file(argv[1])) { printf("cannot load %s\n", argv[1]); return 1; }
    float maxsec = argc > 2 ? (float)atof(argv[2]) : 20.0f, start = argc > 3 ? (float)atof(argv[3]) : 0.0f;
    static const char *NAMES[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    int nk = 0, ns = 0, nh = 0;
    for (double t = start; t < start + maxsec; t += 1.0 / 60.0) {          /* 60 fps, like the game */
        an_at(t);
        float k, s, h; an_drums(&k, &s, &h); nk += k > 0; ns += s > 0; nh += h > 0;
        int tick = (int)((t - start) * 60.0 + 0.5);
        if (tick % 30 == 29) {
            float nt[AN_NOTES], b, m, l; an_notes(nt); an_registers(&b, &m, &l);
            printf("%5.1fs drums(k%d s%d h%d) bass %.2f mid %.2f lead %.2f  notes:", t, nk, ns, nh, b, m, l);
            nk = ns = nh = 0;
            for (int i = 0; i < AN_NOTES; i++) if (nt[i] > 0) printf(" %s%d", NAMES[(AN_NOTE_LO + i) % 12], (AN_NOTE_LO + i) / 12 - 1);
            printf("\n");
        }
    }
    return 0;
}
