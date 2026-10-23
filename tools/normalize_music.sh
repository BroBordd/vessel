#!/bin/bash
# Vessel - Copyright (C) 2026 BroBordd
# SPDX-License-Identifier: GPL-3.0-only (see LICENSE)
# Gives every music/*.ogg the same loudness: EBU R128 integrated loudness, target -17 LUFS, by plain gain (no limiter, no
# compression). A track that would need more gain than its true peak allows (ceiling -1 dBTP) is left a little quieter instead,
# so every track ends up within about 1 LU of the target. Tracks already within 0.3 dB are skipped, so re-running is safe
# and does not re-encode (re-encoding costs a little quality each time).
#   tools/normalize_music.sh [music/]        needs ffmpeg with libvorbis
set -e
DIR=${1:-music}; TARGET=${TARGET:--17}; CEIL=-1
for f in "$DIR"/*.ogg; do
  m=$(ffmpeg -nostats -i "$f" -af ebur128=peak=true -f null - 2>&1 | awk '/^ *I:/{i=$2} /Peak:/{p=$2} END{print i, p}')
  set -- $m; I=$1; P=$2
  g=$(awk -v T="$TARGET" -v I="$I" -v P="$P" -v C="$CEIL" 'BEGIN{g=T-I; c=C-P; if(c<g)g=c; printf "%.2f", g}')
  if awk -v g="$g" 'BEGIN{exit !(g>-0.3 && g<0.3)}'; then echo "keep   $(basename "$f")  ($I LUFS)"; continue; fi
  # re-encode as VBR quality: the lowest quality whose file is at least as big as the original (never smaller, so no quality loss
  # beyond the unavoidable generation); -b:a would not do, it produced files 25-35% smaller than the originals
  osz=$(stat -c %s "$f")
  for q in 5 5.5 6 6.5 7 7.5 8; do
    ffmpeg -y -loglevel error -i "$f" -af "volume=${g}dB" -c:a libvorbis -q:a "$q" -map_metadata 0 "$f.tmp.ogg"
    [ "$(stat -c %s "$f.tmp.ogg")" -ge "$osz" ] && break
  done
  mv "$f.tmp.ogg" "$f"
  echo "gain   $(basename "$f")  $I -> $(awk -v I="$I" -v g="$g" 'BEGIN{printf "%.1f",I+g}') LUFS  (${g} dB, q$q)"
done
