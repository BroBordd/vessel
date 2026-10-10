/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
 *
 * GPU drawing, game side.
 *
 * The game never draws pixels itself in GPU mode. Every SDL render call the game makes
 * (fill rects, colour, blend, clip, clear, one render-target pass for the talk zoom) is
 * recorded as a 16-byte record into a shared file, and the Java app replays the list on the
 * GPU with OpenGL ES (GfxRenderer.java). The layout below must match GfxRenderer.java.
 *
 * Build with  -DGFX_REDIRECT -include game/gfx.h  and the SDL_Render* calls below are routed
 * through this module. Without the flag (tools/preview.c, tests) nothing changes: real SDL.
 * At run time the game picks the mode from argv: "gpu" records, anything else draws on the
 * CPU exactly as before (the same wrappers just forward to SDL).
 */
#ifndef GFX_H
#define GFX_H

#include <SDL2/SDL.h>
#include <stdint.h>

/* ---- shared file layout (little endian, 4-byte ints) -------------------------------------
 * header, 64 bytes:  [0] magic  [1] W  [2] H  [3] nslots  [4] cap (records per slot)
 *                    [5] latest slot  [6] frame counter (written last)  [7] slot the app is reading
 * slot s at 64 + s * GFX_SLOT_BYTES:
 *   16-byte slot header: [0] gen (0 while being written, else the frame number)  [1] nrec
 *   then nrec records of 16 bytes (4 ints: a b c type)
 * records:
 *   type 0 RECT   a = (x+32768) | (y+32768) << 16     b = w | h << 16     c = r | g << 8 | b << 16 | a << 24
 *   type 1 BLEND  a = 0 none / 1 alpha blend
 *   type 2 CLIP   a = (x+32768) | (y+32768) << 16     b = w | h << 16     c = 1 enabled / 0 disabled
 *   type 3 TARGET a = 0 scene / 1 zoom texture
 *   type 4 COPY   a = (x+32768) | (y+32768) << 16     b = w | h << 16     (zoom texture -> whole current target)
 *   type 5 CLEAR  c = colour (whole target, ignores clip and blend)
 * Every frame starts with BLEND, CLIP and TARGET records so replaying it needs no earlier state. */
#define GFX_MAGIC       0x31584647u      /* "GFX1" */
#define GFX_HDR_BYTES   64
#define GFX_NSLOTS      4
#define GFX_CAP         65536
#define GFX_SLOT_HDR    16
#define GFX_REC_BYTES   16
#define GFX_SLOT_BYTES  (GFX_SLOT_HDR + GFX_CAP * GFX_REC_BYTES)
#define GFX_FILE_BYTES  (GFX_HDR_BYTES + GFX_NSLOTS * GFX_SLOT_BYTES)

enum { GFX_RECT = 0, GFX_BLEND = 1, GFX_CLIP = 2, GFX_TARGET = 3, GFX_COPY = 4, GFX_CLEAR = 5 };

/* COLOUR FILTER (the death cutscene): every colour the game sets from now on is passed through it, so one
 * call can turn a whole drawing pass grey and the next one red. it works in both modes (the recorded colour is
 * already filtered, the Java side needs to know nothing) but only for code built with GFX_REDIRECT.
 *   grey 0..256: how far toward the colour's own brightness (and a touch darker, so greyed things sit back)
 *   red  0..256: how far toward a bloody red (greens and blues drain, reds stay)
 * (0, 0) = off. colours set BEFORE the call are not touched: set the filter, then draw. */
void gfx_set_filter(int grey, int red);

/* GPU mode: map the shared file (created by the app) and start recording. 0 on success. */
int  gfx_init_gpu(const char *path, int w, int h);
int  gfx_is_gpu(void);
/* the renderer pointer the game passes around: in GPU mode there is no SDL renderer, only this stand-in */
SDL_Renderer *gfx_dummy_renderer(void);
/* rects drawn in the most recent frame(s): max since the last call, then reset (for the fps log line) */
unsigned gfx_rect_stat(void);

int  gfx_SetRenderDrawColor(SDL_Renderer *r, Uint8 R, Uint8 G, Uint8 B, Uint8 A);
int  gfx_SetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode m);
int  gfx_RenderFillRect(SDL_Renderer *r, const SDL_Rect *rc);
int  gfx_RenderFillRects(SDL_Renderer *r, const SDL_Rect *rc, int n);
int  gfx_RenderClear(SDL_Renderer *r);
int  gfx_RenderSetClipRect(SDL_Renderer *r, const SDL_Rect *rc);
int  gfx_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t);
SDL_Texture *gfx_CreateTexture(SDL_Renderer *r, Uint32 fmt, int access, int w, int h);
void gfx_DestroyTexture(SDL_Texture *t);
int  gfx_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *src, const SDL_Rect *dst);
void gfx_RenderPresent(SDL_Renderer *r);

#ifdef GFX_REDIRECT
#define SDL_SetRenderDrawColor      gfx_SetRenderDrawColor
#define SDL_SetRenderDrawBlendMode  gfx_SetRenderDrawBlendMode
#define SDL_RenderFillRect          gfx_RenderFillRect
#define SDL_RenderFillRects         gfx_RenderFillRects
#define SDL_RenderClear             gfx_RenderClear
#define SDL_RenderSetClipRect       gfx_RenderSetClipRect
#define SDL_SetRenderTarget         gfx_SetRenderTarget
#define SDL_CreateTexture           gfx_CreateTexture
#define SDL_DestroyTexture          gfx_DestroyTexture
#define SDL_RenderCopy              gfx_RenderCopy
#define SDL_RenderPresent           gfx_RenderPresent
#endif

#endif
