/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * LISTENS to the music that is being played and works out what is in it, for the music window:
 *   - which piano notes sound right now (peak picking on a long FFT)
 *   - when the drums hit (kick / snare / hi-hat, by onset detection in their frequency ranges)
 *   - how loud the bass / middle / lead registers are (derived from the notes)
 * an .ogg is one mixed stream, so this is detection, not real stem separation: it is a good
 * guess from the sound itself. no SDL in here, so it can be tested on a PC.
 * feed it from the audio thread, read it from the game thread (plain floats, no locks). */
#ifndef ANALYZE_H
#define ANALYZE_H

#define AN_NOTE_LO  36          /* MIDI note of the lowest key we detect: C2 */
#define AN_NOTES    61          /* 36 .. 96 = C2 .. C7 */
#define AN_RATE     44100

void an_reset(void);
/* pcm: interleaved stereo S16, `frames` long. silence (frames == 0) is fine and lets everything fade */
void an_feed(const short *pcm, int frames);
/* levels 0..1 for MIDI notes AN_NOTE_LO .. AN_NOTE_LO + AN_NOTES - 1 */
void an_notes(float *out);
/* strongest hit of each drum since the last call (0 = none), then cleared */
void an_drums(float *kick, float *snare, float *hat);
/* loudest note (0..1) in the bass (below C3), middle (C3 .. B4) and lead (C5 and up) registers */
void an_registers(float *bass, float *mid, float *lead);

#endif
