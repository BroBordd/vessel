# Incremental build of build/game (used by scripts/run.sh).
# One object per game/*.c in build/obj/, header deps tracked with -MMD, so only changed files (and the
# files that include a changed header) are recompiled. A change to the compiler flags rebuilds everything.
#   make -f scripts/game.mk [-j8] [SDL=$HOME/sdl-static]
SDL    ?= $(HOME)/sdl-static
CC     ?= cc
OBJDIR := build/obj

SRCS := $(wildcard game/*.c)
OBJS := $(SRCS:game/%.c=$(OBJDIR)/%.o) $(OBJDIR)/iconv_stub.o

SDLCONF   := $(SDL)/bin/sdl2-config
SDLCFLAGS := $(shell $(SDLCONF) --cflags | tr ' ' '\n' | grep -vE 'iconv|pipewire|samplerate' | tr '\n' ' ')
SDLLIBS   := $(shell $(SDLCONF) --cflags --static-libs | tr ' ' '\n' | grep -vE 'iconv|pipewire|samplerate' | tr '\n' ' ')

# -DGFX_REDIRECT -include game/gfx.h: the SDL render calls go through game/gfx.c
CFLAGS := -O2 -U__ANDROID__ -DGFX_REDIRECT -include game/gfx.h $(SDLCFLAGS)

build/game: $(OBJS)
	$(CC) -s -o $@ $(OBJS) $(SDLLIBS) -lm -Wl,--as-needed

# stamp file: rewritten only when the flags change, so every object depends on the flags
$(OBJDIR)/.flags: FORCE
	@mkdir -p $(OBJDIR)
	@echo '$(CC) $(CFLAGS)' | cmp -s - $@ || echo '$(CC) $(CFLAGS)' > $@

$(OBJDIR)/%.o: game/%.c $(OBJDIR)/.flags
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR)/iconv_stub.o: scripts/iconv_stub.c $(OBJDIR)/.flags
	$(CC) -O2 -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: FORCE
FORCE:
