/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 * the vessel generator: see vessel_gen.h. everything is integer maths so the same index gives the same vessel on every device. */
#include "vessel_gen.h"
#include <string.h>

/* ---------- a tiny seeded generator (splitmix32), one per vessel: the same on every device ---------- */
typedef struct { uint32_t s; } Rng;
static uint32_t rng_next(Rng *r) {
    uint32_t z = (r->s += 0x9E3779B9u);
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}
static int rng_int(Rng *r, int n) { return (int)(rng_next(r) % (uint32_t)n); }                 /* 0 .. n-1 */
static int rng_in(Rng *r, int lo, int hi) { return lo + rng_int(r, hi - lo + 1); }              /* lo .. hi */

/* ---------- colours ---------- */
static int clamp8(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
static Rgb rgb(int r, int g, int b) { Rgb c = { (Uint8)clamp8(r), (Uint8)clamp8(g), (Uint8)clamp8(b) }; return c; }
/* NB every random draw is its own statement: the order C evaluates the arguments of a call in is unspecified, so a draw inside an argument list
 * could give a different vessel on another compiler or cpu. */
static Rgb jitter(Rng *r, Rgb c, int amt) {                                                      /* a few steps off in every channel: no two vessels share a palette */
    int dr = rng_in(r, -amt, amt), dg = rng_in(r, -amt, amt), db = rng_in(r, -amt, amt);
    return rgb(c.r + dr, c.g + dg, c.b + db);
}
static int dist(Rgb a, Rgb b) {
    int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return (dr < 0 ? -dr : dr) + (dg < 0 ? -dg : dg) + (db < 0 ? -db : db);
}
/* hue 0..359, saturation and value 0..255 -> rgb, integers only */
static Rgb hsv(int h, int s, int v) {
    int region = (h % 360) / 60, rem = ((h % 360) % 60) * 255 / 60;
    int p = v * (255 - s) / 255, q = v * (255 - s * rem / 255) / 255, t = v * (255 - s * (255 - rem) / 255) / 255;
    switch (region) {
    case 0:  return rgb(v, t, p);
    case 1:  return rgb(q, v, p);
    case 2:  return rgb(p, v, t);
    case 3:  return rgb(p, q, v);
    case 4:  return rgb(t, p, v);
    default: return rgb(v, p, q);
    }
}
static Rgb hsv_in(Rng *r, int h0, int h1, int s0, int s1, int v0, int v1) {                      /* a random colour: hue, saturation, value, drawn in that order */
    int h = rng_in(r, h0, h1), sat = rng_in(r, s0, s1), v = rng_in(r, v0, v1);
    return hsv(h, sat, v);
}

static const Rgb HAIR_NATURAL[] = {
    { 24, 22, 26 }, { 46, 34, 28 }, { 96, 58, 36 }, { 130, 84, 50 }, { 160, 100, 48 }, { 196, 150, 70 },
    { 232, 204, 110 }, { 182, 62, 38 }, { 150, 70, 44 }, { 120, 120, 128 }, { 222, 224, 232 } };
static const Rgb HAIR_DYED[] = { { 30, 38, 76 }, { 60, 120, 170 }, { 70, 140, 100 }, { 170, 60, 150 }, { 230, 110, 40 } };
static const Rgb SKIN[] = {
    { 255, 226, 204 }, { 248, 208, 170 }, { 244, 200, 164 }, { 234, 186, 150 }, { 226, 172, 132 }, { 214, 164, 120 },
    { 190, 134, 96 }, { 170, 120, 90 }, { 150, 100, 72 }, { 130, 86, 60 }, { 110, 72, 50 } };
static const Rgb BOOTS[] = {
    { 40, 30, 30 }, { 70, 44, 30 }, { 96, 64, 40 }, { 236, 236, 236 }, { 170, 40, 60 }, { 220, 120, 40 }, { 30, 50, 90 } };
static const Rgb WORN[] = {                                                                       /* caps, beanies, scarves: dull, plain colours */
    { 52, 62, 82 }, { 66, 66, 70 }, { 104, 86, 68 }, { 84, 96, 72 }, { 120, 72, 66 }, { 146, 142, 132 }, { 72, 94, 108 }, { 36, 36, 40 } };
#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* ---------- names ---------- */
/* the index goes through a bijection (so names never repeat) and is spelt as syllables: [body syllables, 64 of them each] + [an ending, 16 of them].
 * the first class has 1 body syllable (16 * 64 = 1024 names), the next 2 (65536), then 3, 4, 5: 5 body syllables holds up to 2^32 and more. */
static const char *ONSET[16] = { "b", "d", "f", "g", "h", "k", "l", "m", "n", "r", "s", "t", "v", "z", "j", "w" };
static const char *VOWEL[4]  = { "a", "e", "i", "o" };
static const char *ENDING[16] = { "on", "us", "ar", "el", "an", "or", "in", "ix", "os", "en", "ax", "ir", "um", "ek", "ul", "as" };

static uint64_t mix_bits(uint64_t r, int bits) {                                                 /* a bijection on 0 .. 2^bits - 1: xorshift and odd multiplies, all invertible */
    uint64_t mask = bits >= 64 ? ~(uint64_t)0 : (((uint64_t)1 << bits) - 1);
    r &= mask; r ^= r >> (bits / 2 + 1);
    r = (r * 0x9E3779B97F4A7C15ull) & mask; r ^= r >> (bits / 3 + 1);
    r = (r * 0xC2B2AE3D27D4EB4Full) & mask; r ^= r >> (bits / 2);
    return r;
}

void vessel_gen_name(uint32_t index, char *buf, int n) {
    if (n <= 0) return;
    uint64_t rank = index;
    int body = 1;                                                                                /* body syllables of this class */
    for (;;) {
        int bits = 6 * body + 4;                                                                 /* 64^body * 16 names in the class */
        uint64_t size = (uint64_t)1 << bits;
        if (rank < size) { rank = mix_bits(rank, bits); break; }
        rank -= size; body++;
    }
    char name[40]; int len = 0;
    for (int i = 0; i < body; i++) {                                                             /* the high digits first */
        int digit = (int)((rank >> (4 + 6 * (body - 1 - i))) & 63);
        const char *o = ONSET[digit >> 2], *v = VOWEL[digit & 3];
        name[len++] = o[0]; name[len++] = v[0];
    }
    const char *e = ENDING[rank & 15];
    while (*e) name[len++] = *e++;
    name[len] = 0;
    if (name[0] >= 'a' && name[0] <= 'z') name[0] = (char)(name[0] - 'a' + 'A');
    strncpy(buf, name, (size_t)n - 1); buf[n - 1] = 0;
}

/* ---------- the vessel ---------- */
void vessel_gen(uint32_t index, VesselLook *out) {
    Rng rng = { index * 0x9E3779B1u + 0x7F4A7C15u };
    for (int i = 0; i < 4; i++) rng_next(&rng);                                                  /* warm up: small indexes must not look alike */
    Person *p = &out->p;
    memset(p, 0, sizeof *p);

    /* the body colours */
    int dyed = rng_int(&rng, 100) < 14;
    p->hair = jitter(&rng, dyed ? HAIR_DYED[rng_int(&rng, N(HAIR_DYED))] : HAIR_NATURAL[rng_int(&rng, N(HAIR_NATURAL))], 10);
    p->skin = jitter(&rng, SKIN[rng_int(&rng, N(SKIN))], 8);
    p->boots = jitter(&rng, BOOTS[rng_int(&rng, N(BOOTS))], 10);
    for (int tries = 0; tries < 8; tries++) {                                                    /* shirt and pants: told apart from each other, and the shirt from the skin */
        p->shirt = hsv_in(&rng, 0, 359, 90, 210, 140, 245);
        int kind = rng_int(&rng, 4);
        if (kind < 2)       p->pants = hsv_in(&rng, 200, 240, 80, 160, 80, 170);   /* denim */
        else if (kind == 2) p->pants = hsv_in(&rng, 0, 359, 0, 40, 30, 110);         /* dark neutrals */
        else                p->pants = hsv_in(&rng, 25, 50, 60, 120, 130, 200);    /* khaki, earth */
        if (dist(p->shirt, p->pants) >= 120 && dist(p->shirt, p->skin) >= 90) break;
    }

    /* the looks: plain ones only, a cap or a beanie (or neither), a scarf, a beard */
    int hat = rng_int(&rng, 100);
    if (hat < 20)       p->acc |= ACC_CAP;
    else if (hat < 35)  p->acc |= ACC_BEANIE;
    if (rng_int(&rng, 100) < 22) p->acc |= ACC_SCARF;
    if (rng_int(&rng, 100) < 22) p->acc |= ACC_BEARD;

    /* the colours of the looks (drawn always, so the rolls stay in step whatever the looks are) */
    for (int tries = 0; tries < 6; tries++) { p->hat = WORN[rng_int(&rng, N(WORN))]; if (dist(p->hat, p->hair) >= 60 && dist(p->hat, p->shirt) >= 60) break; }
    for (int tries = 0; tries < 6; tries++) { p->scarf = WORN[rng_int(&rng, N(WORN))]; if (dist(p->scarf, p->shirt) >= 60) break; }

    p->long_hair = 0;
    vessel_gen_name(index, out->name, VESSEL_NAME_MAX);
    p->name = out->name;
}

/* ---------- the kept ones ---------- */
#define KEEP 16
const Person *vessel_gen_person(uint32_t index) {
    static VesselLook slot[KEEP];
    static uint32_t idx[KEEP];
    static int used[KEEP], next;
    for (int i = 0; i < KEEP; i++) if (used[i] && idx[i] == index) return &slot[i].p;
    int i = next; next = (next + 1) % KEEP;
    vessel_gen(index, &slot[i]);
    idx[i] = index; used[i] = 1;
    return &slot[i].p;
}
