# Fujitzee for the ColecoVision.
#
#   make            build/fujitzee.bin
#   make run        build it and run it in MAME against a fujinet-pc
#   make assets     regenerate assets/charset.inc from src/msdos/charset.c
#   make smoke      headless: does the splash come up on a real cartridge?
#   make shot       capture build/splash.png -- the RENDERED screen
#   make clean

.PHONY: all run assets smoke shot clean

all:
	./build.sh

run: all
	./run.sh

assets:
	python3 tools/mkcharset.py

# The one check that needs no server: the ROM boots, Graphics II comes up, the
# charset uploads, and FNCHECK finds a cartridge underneath. Everything past
# the splash needs a fujinet-pc and a game server, so it lives in run.sh.
smoke: all
	SCREEN_AT=3 NOTHROTTLE=1 ./run.sh --headless "PRESS FIRE"

# The name table is not the screen. A VDP left in the wrong mode, or a charset
# that only half uploaded, reads back perfectly out of VRAM and displays
# nothing at all -- both of which happened here. This captures pixels.
shot: all
	SNAPOUT=$(CURDIR)/build/splash.png SCREEN_AT=3 NOTHROTTLE=1 \
	    ./run.sh --headless "PRESS FIRE"
	@echo "wrote build/splash.png"

clean:
	$(RM) -r build
