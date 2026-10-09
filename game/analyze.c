/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
#include "analyze.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NN 8192                       /* long window for notes: 5.4 Hz per bin */
#define DN 2048                       /* short window for drums: 21.5 Hz per bin, 46 ms */

static float ring[NN];                /* last NN mono samples */
static int   rpos;
static int   tick;

static volatile float note_v[AN_NOTES];
static volatile float drum_hit[3];
static float prev[DN / 2];
static float norm[3] = { 1.2f, 1.2f, 1.2f };

static void fft(float *re, float *im, int n) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * (float)M_PI / len, wr = cosf(ang), wi = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float cr = 1, ci = 0;
            for (int k = 0; k < len / 2; k++) {
                int a = i + k, b = i + k + len / 2;
                float xr = re[b] * cr - im[b] * ci, xi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - xr; im[b] = im[a] - xi;
                re[a] += xr;        im[a] += xi;
                float nr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = nr;
            }
        }
    }
}

void an_reset(void) {
    memset(ring, 0, sizeof ring); rpos = 0; tick = 0;
    for (int i = 0; i < AN_NOTES; i++) note_v[i] = 0;
    for (int i = 0; i < 3; i++) { drum_hit[i] = 0; norm[i] = 1.2f; }
    memset(prev, 0, sizeof prev);
}

static float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

/* ---------- drums: positive spectral flux (how much each range just got louder) ---------- */
static void analyze_drums(void) {
    static float re[DN], im[DN], win[DN]; static int wready;
    if (!wready) { for (int i = 0; i < DN; i++) win[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (DN - 1)); wready = 1; }
    for (int i = 0; i < DN; i++) { re[i] = ring[(rpos - DN + i + NN) % NN] * win[i]; im[i] = 0; }
    fft(re, im, DN);
    /* band edges in bins of 21.5 Hz: kick 43-150 Hz, snare body 190-540 Hz + its noise 2-6 kHz, hats 7-15 kHz */
    static const int lo[4] = { 2, 9, 95, 325 }, hi[4] = { 7, 26, 280, 700 };
    float flux[4] = { 0, 0, 0, 0 };
    for (int k = 1; k < DN / 2; k++) {
        float mag = sqrtf(re[k] * re[k] + im[k] * im[k]) / (DN / 4.0f);
        float c = log10f(1.0f + 300.0f * mag);
        float d = c - prev[k]; prev[k] = c;
        if (d <= 0) continue;
        for (int b = 0; b < 4; b++) if (k >= lo[b] && k < hi[b]) flux[b] += d;
    }
    float f[3] = { flux[0], flux[1] * 0.6f + flux[2] * 0.4f, flux[3] };
    for (int b = 0; b < 3; b++) {
        float floor_ = b == 0 ? 1.0f : b == 1 ? 1.6f : 1.4f;         /* below this it is just wobble, not a hit */
        norm[b] *= 0.996f;                                            /* slowly forget loud passages */
        if (f[b] > norm[b]) norm[b] = f[b];
        if (norm[b] < floor_) norm[b] = floor_;
        float rel = f[b] / norm[b];
        if (f[b] < floor_ * 0.55f || rel < 0.45f) continue;           /* ignore small flutter */
        float hit = clamp01((rel - 0.35f) / 0.65f);
        if (hit > drum_hit[b]) drum_hit[b] = hit;
    }
}

/* ---------- notes: strongest narrow peaks of the long spectrum, per piano key ---------- */
static void analyze_notes(void) {
    static float re[NN], im[NN], win[NN]; static int wready;
    if (!wready) { for (int i = 0; i < NN; i++) win[i] = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * i / (NN - 1)); wready = 1; }
    for (int i = 0; i < NN; i++) { re[i] = ring[(rpos + i) % NN] * win[i]; im[i] = 0; }
    fft(re, im, NN);
    static float mag[NN / 2];
    for (int k = 0; k < NN / 2; k++) mag[k] = sqrtf(re[k] * re[k] + im[k] * im[k]) / (NN / 4.0f);

    float s[AN_NOTES + 2];                         /* a spare slot on each side so neighbours always exist */
    float best = 0;
    for (int i = 0; i < AN_NOTES; i++) {
        float f = 440.0f * powf(2.0f, (float)(AN_NOTE_LO + i - 69) / 12.0f);
        float c = f * NN / AN_RATE, w = c * 0.0293f; if (w < 0.6f) w = 0.6f;       /* +- a quarter tone */
        int a = (int)(c - w + 0.5f), b = (int)(c + w + 0.5f);
        if (a < 1) a = 1;
        if (b >= NN / 2) b = NN / 2 - 1;
        float pk = 0;
        for (int k = a; k <= b; k++) if (mag[k] > pk) pk = mag[k];
        float db = 20.0f * log10f(pk + 1e-9f);
        s[i + 1] = clamp01((db + 64.0f) / 34.0f);
        if (s[i + 1] > best) best = s[i + 1];
    }
    s[0] = s[AN_NOTES + 1] = 0;
    float out[AN_NOTES];
    for (int i = 0; i < AN_NOTES; i++) {
        float v = s[i + 1];
        int peak = v >= s[i] && v >= s[i + 2];                         /* a note is a peak among its neighbours */
        if (!peak || v < 0.30f || v < 0.45f * best) { out[i] = 0; continue; }
        if (i >= 12 && s[i + 1 - 12] > v * 1.12f) v *= 0.45f;          /* probably the overtone of a note an octave down */
        out[i] = v;
    }
    for (int i = 0; i < AN_NOTES; i++) note_v[i] = out[i];
}


/* ---------- precomputed data: the .vsd file ----------
 * little endian:  "VSD1" | u32 notes | u32 hits | u32 duration_ms
 *   notes (sorted by start): u32 start_ms, u16 dur_ms, u8 midi, u8 velocity   (8 bytes each)
 *   hits  (sorted by time):  u32 ms, u8 drum (0 kick, 1 snare, 2 hat), u8 strength   (6 bytes each) */
typedef struct { unsigned start, dur; int midi; float vel; } VNote;
typedef struct { unsigned ms; int drum; float str; } VHit;
static VNote *vn; static int nvn; static unsigned vmaxdur;
static VHit  *vh; static int nvh;
static volatile int vsd_on;                    /* the audio thread only ever looks at this flag */
static char vsd_name[128];                     /* what an_track() last tried, found or not */
static double vsd_last = -1.0;

static unsigned rd32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }

static void vsd_unload(void) {
    vsd_on = 0;
    free(vn); free(vh); vn = NULL; vh = NULL; nvn = nvh = 0; vmaxdur = 0; vsd_last = -1.0;
}

int an_load_file(const char *path) {
    vsd_unload();
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char hd[16];
    if (fread(hd, 1, 16, f) != 16 || memcmp(hd, "VSD1", 4) != 0) { fclose(f); return 0; }
    unsigned n = rd32(hd + 4), m = rd32(hd + 8);
    if (n > 200000 || m > 200000) { fclose(f); return 0; }
    unsigned char *nb = malloc((size_t)n * 8 + 1), *hb = malloc((size_t)m * 6 + 1);
    VNote *a = malloc(sizeof(VNote) * (n + 1)); VHit *b = malloc(sizeof(VHit) * (m + 1));
    int ok = nb && hb && a && b && fread(nb, 8, n, f) == n && fread(hb, 6, m, f) == m;
    fclose(f);
    if (!ok) { free(nb); free(hb); free(a); free(b); return 0; }
    for (unsigned i = 0; i < n; i++) {
        const unsigned char *q = nb + i * 8;
        int midi = q[6];
        while (midi < AN_NOTE_LO) midi += 12;                 /* fold what is outside the keyboard back onto it */
        while (midi >= AN_NOTE_LO + AN_NOTES) midi -= 12;
        a[i].start = rd32(q); a[i].dur = q[4] | q[5] << 8; a[i].midi = midi; a[i].vel = q[7] / 255.0f;
        if (a[i].dur < 30) a[i].dur = 30;
        if (a[i].dur > vmaxdur) vmaxdur = a[i].dur;
    }
    for (unsigned i = 0; i < m; i++) {
        const unsigned char *q = hb + i * 6;
        b[i].ms = rd32(q); b[i].drum = q[4] > 2 ? 2 : q[4]; b[i].str = q[5] / 255.0f;
    }
    free(nb); free(hb);
    vn = a; nvn = (int)n; vh = b; nvh = (int)m;
    for (int i = 0; i < AN_NOTES; i++) note_v[i] = 0;
    vsd_on = 1;
    return 1;
}

int an_track(const char *ogg) {
    if (strcmp(ogg, vsd_name) == 0) return vsd_on;           /* same track as last time (found or not) */
    snprintf(vsd_name, sizeof vsd_name, "%s", ogg);
    vsd_unload();
    for (int i = 0; i < AN_NOTES; i++) note_v[i] = 0;
    if (!ogg[0]) return 0;
    char base[256]; snprintf(base, sizeof base, "%s", ogg);
    char *dot = strrchr(base, '.'); if (dot) *dot = 0;
    char path[1200], dir[900];
    ssize_t n = readlink("/proc/self/exe", dir, sizeof dir - 1);       /* same lookup order as music_play */
    if (n > 0) {
        dir[n] = 0;
        char *slash = strrchr(dir, '/');
        if (slash) { *slash = 0; snprintf(path, sizeof path, "%s/%s.vsd", dir, base); if (an_load_file(path)) return 1; }
    }
    snprintf(path, sizeof path, "%s.vsd", base);
    return an_load_file(path);
}

int an_has_data(void) { return vsd_on; }

void an_at(double pos) {
    if (!vsd_on) return;
    if (pos < 0) {                                           /* nothing audible */
        for (int i = 0; i < AN_NOTES; i++) note_v[i] = 0;
        vsd_last = -1.0;
        return;
    }
    unsigned t = (unsigned)(pos * 1000.0);
    float lv[AN_NOTES];
    for (int i = 0; i < AN_NOTES; i++) lv[i] = 0;
    unsigned from = t > vmaxdur ? t - vmaxdur : 0;           /* first note that could still be sounding */
    int lo = 0, hi = nvn;
    while (lo < hi) { int mid = (lo + hi) / 2; if (vn[mid].start < from) lo = mid + 1; else hi = mid; }
    for (int i = lo; i < nvn && vn[i].start <= t; i++) {
        if (t >= vn[i].start + vn[i].dur) continue;
        float age = (float)(t - vn[i].start) / (float)vn[i].dur;
        float v = (0.35f + 0.65f * vn[i].vel) * (1.0f - 0.35f * age);          /* a note fades a little as it rings */
        int k = vn[i].midi - AN_NOTE_LO;
        if (v > lv[k]) lv[k] = v;
    }
    for (int i = 0; i < AN_NOTES; i++) note_v[i] = lv[i];

    /* drum hits that happened since the last call. a seek / scrub / loop jump resyncs without firing */
    if (vsd_last >= 0 && pos > vsd_last && pos - vsd_last < 0.5) {
        unsigned a = (unsigned)(vsd_last * 1000.0);
        lo = 0; hi = nvh;
        while (lo < hi) { int mid = (lo + hi) / 2; if (vh[mid].ms <= a) lo = mid + 1; else hi = mid; }
        for (int i = lo; i < nvh && vh[i].ms <= t; i++)
            if (vh[i].str > drum_hit[vh[i].drum]) drum_hit[vh[i].drum] = vh[i].str;
    }
    vsd_last = pos;
}

void an_feed(const short *pcm, int frames) {
    if (vsd_on) return;                                 /* precomputed data is in use: nothing to listen for */
    for (int i = 0; i < frames; i++) {
        ring[rpos] = (pcm[i * 2] + pcm[i * 2 + 1]) * (0.5f / 32768.0f);
        rpos = (rpos + 1) % NN;
    }
    if (frames == 0) {                                  /* nothing playing: push one chunk of silence so it all fades */
        for (int i = 0; i < 1024; i++) { ring[rpos] = 0; rpos = (rpos + 1) % NN; }
    }
    analyze_drums();
    if ((tick++ & 1) == 0) analyze_notes();            /* the long one every other chunk (~46 ms) */
}

void an_notes(float *out) { for (int i = 0; i < AN_NOTES; i++) out[i] = note_v[i]; }

void an_drums(float *k, float *s, float *h) {
    *k = drum_hit[0]; *s = drum_hit[1]; *h = drum_hit[2];
    drum_hit[0] = drum_hit[1] = drum_hit[2] = 0;
}

void an_registers(float *bass, float *mid, float *lead) {
    float b = 0, m = 0, l = 0;
    for (int i = 0; i < AN_NOTES; i++) {
        int midi = AN_NOTE_LO + i; float v = note_v[i];
        if (midi < 48) { if (v > b) b = v; } else if (midi < 72) { if (v > m) m = v; } else if (v > l) l = v;
    }
    *bass = b; *mid = m; *lead = l;
}
