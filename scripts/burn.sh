#!/data/data/com.termux/files/usr/bin/bash
set -e
cd "$(dirname "$0")/.."

SDL=$HOME/sdl-static
mkdir -p build dist

cat > build/iconv_stub.c <<'EOF'
#include <stddef.h>
#include <errno.h>
void *libiconv_open(const char *t, const char *f) { errno = EINVAL; return (void *)-1; }
int libiconv_close(void *cd) { return 0; }
size_t libiconv(void *cd, char **i, size_t *il, char **o, size_t *ol) { errno = EILSEQ; return (size_t)-1; }
EOF

LIBS=$($SDL/bin/sdl2-config --cflags --static-libs | tr ' ' '\n' \
  | grep -vE 'iconv|pipewire|samplerate' | tr '\n' ' ')

# PIE executable named .so: Android extracts it to nativeLibraryDir
cc -O2 -s -pie -U__ANDROID__ -o dist/libgame.so game/game.c build/iconv_stub.c \
  $LIBS -lm -Wl,--as-needed

BAD=$(readelf -d dist/libgame.so | grep NEEDED | grep -vE '\[(libc|libm|libdl|liblog)\.so\]' || true)
if [ -n "$BAD" ]; then echo "bad libs:"; echo "$BAD"; exit 1; fi

ls -l dist/libgame.so
git add dist/libgame.so game scripts 2>/dev/null || true
echo "now: git commit -m burn && git push, then in Codespaces: git pull && scripts/build-apk.sh --burn"
