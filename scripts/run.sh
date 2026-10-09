#!/data/data/com.termux/files/usr/bin/bash
set -e
cd "$(dirname "$0")/.."

PKG=io.github.brobordd.vessel
SDL=$HOME/sdl-static
mkdir -p build

command -v readelf >/dev/null || pkg install -y binutils

cat > build/iconv_stub.c <<'EOF'
#include <stddef.h>
#include <errno.h>
void *libiconv_open(const char *t, const char *f) { errno = EINVAL; return (void *)-1; }
int libiconv_close(void *cd) { return 0; }
size_t libiconv(void *cd, char **i, size_t *il, char **o, size_t *ol) { errno = EILSEQ; return (size_t)-1; }
EOF

LIBS=$($SDL/bin/sdl2-config --cflags --static-libs | tr ' ' '\n' \
  | grep -vE 'iconv|pipewire|samplerate' | tr '\n' ' ')

# -DGFX_REDIRECT -include game/gfx.h: the SDL render calls go through game/gfx.c (GPU recording, or plain SDL in cpu mode)
cc -O2 -s -U__ANDROID__ -DGFX_REDIRECT -include game/gfx.h -o build/game game/*.c build/iconv_stub.c \
  $LIBS -lm -Wl,--as-needed

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
