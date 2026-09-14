# Makefile -- a convenience wrapper. build.sh is the real build.
#
# The root build (mekkogx) drives the cc65/cmoc-family C toolchains; this port
# is 6502 assembly under Macroassembler AS, which is a different toolchain
# entirely, so it stands alone next to intv/, arcadia/, astrocade/, coleco/
# and channelf/ rather than being forced through machinery it would only
# complicate.

ENDPOINT ?= https://fujitzee.carr-designs.com/
FUJI_FIRMWARE ?= $(HOME)/Workspace/fujinet-firmware
MAME_DIR ?= $(HOME)/Workspace/mame
SECS ?= 240

# MAME NUMBERS SNAPSHOTS UPWARD rather than overwriting, so every target that
# reads one back clears the directory first -- otherwise the checker reads
# whatever an earlier target left at 0000.png and reports on the wrong screen.
SRC := $(wildcard src/*.asm src/*.inc)
TOOLS := tools/checkbanks.py tools/mktail.py tools/mkdice.py tools/checkart.py

.PHONY: all layout run runlayout shot drive frames longframe standings menu \
        reveal stack inplag curbar holdkeep dicecheck gameover emu clean

all: build/fujitzee.bin

build/fujitzee.bin: $(SRC) build.sh $(TOOLS)
	ENDPOINT="$(ENDPOINT)" FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh

# The whole scorecard with no network at all: the kernel, the per-row colours,
# the green potentials, the dice band and the cursor, from a screen baked into
# the image. It runs in the STOCK cart slot -- there is no mailbox in it.
layout: build/layout.bin
build/layout.bin: $(SRC) build.sh tools/mklayout.py
	ENDPOINT="$(ENDPOINT)" FUJI_FIRMWARE="$(FUJI_FIRMWARE)" ./build.sh layout

run: all
	MAME=$(MAME_DIR) ./run.sh fujitzee
runlayout: layout
	MAME=$(MAME_DIR) ./run.sh layout

# A snapshot of the layout ROM, read back CELL BY CELL through the same font
# the cartridge composes with, plus every die against the art generator.
shot: layout
	rm -rf build/snap
	MAME=$(MAME_DIR) FAST=1 SECS=8 ./run.sh layout shot
	python3 emu/scrcheck.py build/snap/a2600/0000.png

# Headless, against the live server: join, ready up, roll, let the cursor park
# itself, take the row it parked on, and go round again. NOT -nothrottle: the
# server's start countdown and its move clock are wall clock.
drive: all
	MAME=$(MAME_DIR) SECS=$(SECS) ./run.sh fujitzee drive

# Every frame across a played game must be 262 lines. This is the gate that
# catches a seam line that quietly took two scanlines, a compose that overran
# its vblank, and a bank switched into from a hook that started a fresh frame.
frames: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee frames
# ...and when one does, which bank was mapped when it happened.
longframe: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee longframe

# The three screens that are not the card.
standings: all
	rm -rf build/snap
	MAME=$(MAME_DIR) SECS=60 DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee standings
	python3 emu/readscr.py build/snap/a2600/0000.png
menu: all
	rm -rf build/snap
	MAME=$(MAME_DIR) SECS=60 DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee menu
	python3 emu/readscr.py build/snap/a2600/0000.png
# A bot's chosen row, held up on their card. Needs a whole game to find one.
reveal: all
	rm -rf build/snap
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee reveal
	python3 emu/readscr.py build/snap/a2600/0000.png

# The stack lives in 46 bytes above the variables; this says it stays there.
stack: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee stack

# A press begun while a poll or a compose is mapped -- which is up to
# twenty-four frames after every poll -- must still be latched within a frame.
inplag: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee inplag
# The dice cursor bar, under the die the stick is actually on. Waits for the
# cursor to be STILL first: MAME's frame boundary is not the program's VSYNC.
curbar: all
	rm -rf build/snap
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee curbar

# A die you have held must STAY held until you roll: keepRoll is an echo of
# the last roll that went out, so restaging from it between rolls throws the
# hold away. The driver sits on the hold for 150 frames, which is longer than
# a poll -- without that wait the bug is invisible.
holdkeep: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee holdkeep

# A roll rattles, and only the dice the wire's keepRoll says actually moved --
# so an opponent's roll animates correctly and not just yours.
dicecheck: all
	MAME=$(MAME_DIR) SECS=$(SECS) DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee dicecheck
# Play all thirteen rounds and snapshot what the server calls it. Slow: a
# whole game against the bots is about eight minutes of wall clock.
gameover: all
	rm -rf build/snap
	MAME=$(MAME_DIR) SECS=$${SECS:-560} DRIVE_LUA=$(PWD)/emu/drive.lua \
	    ./run.sh fujitzee gameover
	python3 emu/readscr.py build/snap/a2600/0000.png

# Re-graft the cartridge device into MAME. Needed after ANY edit to a shared
# firmware source -- MAME compiles its own copies.
emu:
	$(FUJI_FIRMWARE)/pico/atari-2600/emu/apply.sh $(MAME_DIR)
	$(MAKE) -C $(MAME_DIR) -j$$(nproc) NOWERROR=1

clean:
	rm -rf build
