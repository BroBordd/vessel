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
  br=$(ffprobe -v error -select_streams a:0 -show_entries stream=bit_rate -of csv=p=0 "$f")
  [ -z "$br" ] || [ "$br" = N/A ] && br=128000
  ffmpeg -y -loglevel error -i "$f" -af "volume=${g}dB" -c:a libvorbis -b:a "$br" -map_metadata 0 "$f.tmp.ogg"
  mv "$f.tmp.ogg" "$f"
  echo "gain   $(basename "$f")  $I -> $(awk -v I="$I" -v g="$g" 'BEGIN{printf "%.1f",I+g}') LUFS  (${g} dB)"
done
