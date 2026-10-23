#!/data/data/com.termux/files/usr/bin/bash
set -e
cd "$(dirname "$0")/.."

PKG=io.github.brobordd.vessel
SDL=$HOME/sdl-static
mkdir -p build

command -v readelf >/dev/null || pkg install -y binutils

command -v make >/dev/null || pkg install -y make

# incremental: only changed .c files (and files that include a changed header) are recompiled, objects live in build/obj
# JOBS=2 ./scripts/run.sh  to use fewer cores; rm -rf build/obj  to force a full rebuild
TOTAL=$(make -f scripts/game.mk -n SDL="$SDL" | grep -c ' -c ' || true)
rm -f build/.progress
if [ "$TOTAL" = 0 ]; then echo "build: up to date"; else echo "build: compiling $TOTAL file(s)"; fi
make -f scripts/game.mk -j"${JOBS:-$(nproc)}" SDL="$SDL" TOTAL="$TOTAL"

BAD=$(readelf -d build/game | grep NEEDED | grep -vE '\[(libc|libm|libdl|liblog)\.so\]' || true)
if [ -n "$BAD" ]; then
  echo "game links against non-system libs, not deploying:"; echo "$BAD"; exit 1
fi

su -c "
D=/data/data/$PKG/files
if [ ! -d \$D ]; then am start -n $PKG/.MainActivity; sleep 2; fi
am force-stop $PKG
cp $PWD/build/game \$D/game
chown \$(stat -c %u \$D):\$(stat -c %g \$D) \$D/game
chmod 755 \$D/game
restorecon \$D/game
cp $PWD/music/*.ogg \$D/
cp $PWD/music/*.vsd \$D/ 2>/dev/null || true
chown \$(stat -c %u \$D):\$(stat -c %g \$D) \$D/*.ogg \$D/*.vsd 2>/dev/null || true
chmod 644 \$D/*.ogg \$D/*.vsd 2>/dev/null || true
restorecon \$D/*.ogg \$D/*.vsd 2>/dev/null || true
rm -f \$D/fb \$D/gfx \$D/audio.pcm
logcat -c
am start -n $PKG/.MainActivity
"

sleep 3
echo "--- game log ---"
su -c 'logcat -d -s game' | tail -n 20
