/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * the vessel generator (vessel_gen.c): headless checks. the same index always gives the same vessel and name; names never repeat (every index in
 * the first 2 million, and spot checks across the whole 32 bits, are distinct); the looks of consecutive vessels differ; every colour set is
 * unique; each look turns up at about the rate it was asked for; the kept pointers (vessel_gen_person) are stable. exits non-zero if a check fails.
 *   cc -O1 $(sdl2-config --cflags) -Igame -o build/vgentest tools/vgentest.c game/vessel_gen.c && build/vgentest */
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vessel_gen.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static int cmp_u64(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return x < y ? -1 : x > y; }
static uint64_t fnv(const void *data, size_t n) { const unsigned char *d = data; uint64_t h = 1469598103934665603ull; while (n--) { h ^= *d++; h *= 1099511628211ull; } return h; }
static uint64_t look_hash(const Person *p) {                    /* everything that is drawn: colours and looks (not the name pointer) */
    unsigned char b[64]; int n = 0;
    const Rgb *c[] = { &p->hair, &p->skin, &p->shirt, &p->pants, &p->boots, &p->hat, &p->scarf, &p->frame };
    for (int i = 0; i < 8; i++) { b[n++] = c[i]->r; b[n++] = c[i]->g; b[n++] = c[i]->b; }
    b[n++] = (unsigned char)p->acc; b[n++] = (unsigned char)(p->acc >> 8); b[n++] = (unsigned char)p->long_hair;
    return fnv(b, (size_t)n);
}

int main(void) {
    const uint32_t N = 2000000;

    /* determinism */
    for (uint32_t i = 0; i < 2000; i++) {
        VesselLook a, b; vessel_gen(i * 7919u, &a); vessel_gen(i * 7919u, &b);
        CHECK(look_hash(&a.p) == look_hash(&b.p) && !strcmp(a.name, b.name), "vessel %u differs between two calls", i * 7919u);
        CHECK(a.p.name == a.name, "name pointer of %u", i);
    }
    /* a golden value: the same on every device and every build (integer maths only). if this changes, every vessel changed */
    { VesselLook v; vessel_gen(2, &v);
      CHECK(!strcmp(v.name, "Deax") && v.p.hair.r == 168 && v.p.hair.g == 109 && v.p.hair.b == 53 && v.p.acc == (ACC_PLASTER | ACC_SCARF | ACC_BAND),
            "vessel 2 changed: %s hair %d,%d,%d acc %d (every vessel changed with it)", v.name, v.p.hair.r, v.p.hair.g, v.p.hair.b, v.p.acc); }

    /* names: pronounceable, bounded, never repeated */
    uint64_t *nm = malloc(sizeof(uint64_t) * (N + 100000)), *lk = malloc(sizeof(uint64_t) * N);
    long cnt[16] = { 0 };
    uint32_t k = 0;
    for (uint32_t i = 0; i < N; i++) {
        char buf[VESSEL_NAME_MAX]; vessel_gen_name(i, buf, sizeof buf);
        CHECK(buf[0] >= 'A' && buf[0] <= 'Z' && strlen(buf) >= 4 && strlen(buf) < VESSEL_NAME_MAX, "bad name %u '%s'", i, buf);
        nm[k++] = fnv(buf, strlen(buf));
    }
    uint32_t spots[] = { 0xFFFFFFFFu, 0xFFFFFFFEu, 0x80000000u, 0x7FFFFFFFu, 1023u, 1024u, 1025u, 66559u, 66560u, 66561u, 4261411839u, 4261411840u };
    for (size_t i = 0; i < sizeof spots / sizeof *spots; i++) { char buf[VESSEL_NAME_MAX]; vessel_gen_name(spots[i], buf, sizeof buf); if (spots[i] >= N) nm[k++] = fnv(buf, strlen(buf));     /* (the ones below N are in already) */
        CHECK(strlen(buf) >= 4 && strlen(buf) < VESSEL_NAME_MAX, "spot name %u", spots[i]); }
    for (uint32_t i = 0; i < 89000; i++) { char buf[VESSEL_NAME_MAX]; vessel_gen_name(0xF0000000u + i * 2999u,       /* (stays below 2^32: no wrap into the first batch) */ buf, sizeof buf); nm[k++] = fnv(buf, strlen(buf)); }
    qsort(nm, k, sizeof(uint64_t), cmp_u64);
    long dupn = 0; for (uint32_t i = 1; i < k; i++) if (nm[i] == nm[i - 1]) dupn++;
    CHECK(dupn == 0, "%ld repeated names in %u", dupn, k);
    printf("names: %u checked, %ld repeats\n", k, dupn);

    /* looks: unique colour sets, neighbours differ, the rates */
    long same_next = 0; static const char *LN[] = { "cap", "beanie", "band", "scarf", "stripe", "pocket", "beard", "glasses", "plaster" };
    static const int LF[] = { ACC_CAP, ACC_BEANIE, ACC_BAND, ACC_SCARF, ACC_STRIPE, ACC_POCKET, ACC_BEARD, ACC_GLASSES, ACC_PLASTER };
    long lc[9] = { 0 }, nohat = 0, twohats = 0;
    for (uint32_t i = 0; i < N; i++) {
        VesselLook v; vessel_gen(i, &v);
        lk[i] = look_hash(&v.p);
        if (i && lk[i] == lk[i - 1]) same_next++;
        for (int j = 0; j < 9; j++) if (v.p.acc & LF[j]) lc[j]++;
        int hats = !!(v.p.acc & ACC_CAP) + !!(v.p.acc & ACC_BEANIE) + !!(v.p.acc & ACC_BAND);
        if (!hats) nohat++;
        if (hats > 1) twohats++;
        CHECK(!v.p.long_hair, "vessel %u has long hair", i);
        (void)cnt;
    }
    qsort(lk, N, sizeof(uint64_t), cmp_u64);
    long dupl = 0; for (uint32_t i = 1; i < N; i++) if (lk[i] == lk[i - 1]) dupl++;
    CHECK(same_next == 0, "%ld neighbouring vessels look the same", same_next);
    CHECK(dupl == 0, "%ld repeated looks in %u", dupl, N);
    CHECK(twohats == 0, "%ld vessels with two hats", twohats);
    printf("looks: %u vessels, %ld repeated colour sets, %ld equal neighbours, hatless %.1f%%\n", N, dupl, same_next, 100.0 * nohat / N);
    double want[9] = { 20, 15, 15, 22, 20, 20, 22, 24, 8 };
    for (int j = 0; j < 9; j++) {
        double got = 100.0 * lc[j] / N;
        printf("  %-8s %5.1f%% (asked %.0f%%)\n", LN[j], got, want[j]);
        CHECK(got > want[j] - 1.5 && got < want[j] + 1.5, "%s rate %.1f%% vs %.0f%%", LN[j], got, want[j]);
    }

    /* the kept ones */
    const Person *a = vessel_gen_person(12345), *b = vessel_gen_person(12345);
    CHECK(a == b, "kept pointer not stable");
    const Person *keep[10]; for (int i = 0; i < 10; i++) keep[i] = vessel_gen_person(100 + (uint32_t)i);
    CHECK(vessel_gen_person(12345) == a, "kept pointer moved after 10 others");
    for (int i = 0; i < 10; i++) { VesselLook v; vessel_gen(100 + (uint32_t)i, &v); CHECK(look_hash(keep[i]) == look_hash(&v.p) && !strcmp(keep[i]->name, v.name), "kept %d wrong", i); }

    free(nm); free(lk);
    printf(fails ? "vgentest: %d FAILED\n" : "vgentest: all ok\n", fails);
    return fails != 0;
}
