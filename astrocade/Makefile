# Fujitzee for the Bally Astrocade. Standalone, outside mekkogx, in the
# battleship/astrocade mold: the build is build.sh (zmac + the layout
# checks), this file is the convenience wrapper.

SRCS := fujitzee.asm HVGLIB.H fujinet.inc fujilib.inc \
        $(wildcard *.inc) $(wildcard assets/*.inc)

all: build/fujitzee.bin

build/fujitzee.bin: $(SRCS) build.sh tools/checksize.py
	./build.sh

run: build/fujitzee.bin
	./run.sh

MAME_DIR ?= $(HOME)/Workspace/mame

# Headless end-to-end test against a LOCAL server (the table order is
# fixed there): name accept, join the bot table, ready, then a trigger
# every ~4 s plays a whole legal game blind -- the dice cursor rests on
# ROLL so each press re-rolls, and once the rolls run out the cursor
# jumps to the best open row so the next press scores it. Snapshot to
# build/astrocde/0000.png. Needs a running fujinet-pc (BoIP on
# 127.0.0.1:9995) and the fujitzee server on 127.0.0.1:8080, and the
# ROM built with ENDPOINT=http://127.0.0.1:8080/ ./build.sh.
smoke: build/fujitzee.bin
	rm -rf build/astrocde
	FUJINET_DEBUG=$${FUJINET_DEBUG:-1} $(MAME_DIR)/mame astrocde \
	    -rompath $(MAME_DIR)/roms \
	    -cartslot fujinet -cart $(CURDIR)/build/fujitzee.bin \
	    -autoboot_script emu/smoke.lua -video none -sound none \
	    -seconds_to_run 100 -snapshot_directory $(CURDIR)/build
	@ls build/astrocde/

clean:
	rm -rf build

.PHONY: all run clean
