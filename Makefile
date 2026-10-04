PRODUCT = fujitzee
PRODUCT_UPPER = FUJITZEE
PLATFORMS = atari apple2 coco msdos dragon adam

PLATFORM_COMBOS = \
  dragon+=coco

# CoCo targets:
#   make coco        → CoCo 1/2 build
#   make coco3       → CoCo 3 build (320x200x16, MSDOS-style layout)
#   make coco-dist   → combined disk with loader + both CoCo binaries

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# src/platform-specific is needed by everyone (#include "platform-specific/foo.h")
INCLUDE_DIRS = src/platform-specific

# src/include holds wrappers (stdint.h, conio.h ...) for non-cc65 toolchains.
# cc65 ships its own real versions, so adding it globally would shadow them.
EXTRA_INCLUDE_COCO  = src/include
EXTRA_INCLUDE_DRAGON = src/include

# Default fujinet-lib version. msdos requires fujinet-lib-experimental for
# RS232 support; override at build time so it doesn't drag the experimental
# branch into other platforms.
FUJINET_LIB =
ifeq ($(PLATFORM),msdos)
  FUJINET_LIB = https://github.com/FozzTexx/fujinet-lib-experimental.git
endif

# Adam (z88dk): no published Adam release zip yet, so build against the
# sibling fujinet-lib checkout (make -C ../fujinet-lib TARGETS=adam first).
ifeq ($(PLATFORM),adam)
  FUJINET_LIB = ../fujinet-lib/build
endif
LDFLAGS_EXTRA_ADAM = -m

# NES (cc65, FujiNet mailbox cartridge). Not in PLATFORMS: the nes bus exists
# only on the add-nes branch of fujinet-lib-experimental, so it builds against
# that checkout:  make nes   (or FUJINET_LIB=<path> to point elsewhere).
# src/nes is guarded by BUILD_NES; CUSTOM_FUJINET_CALLS routes the network and
# AppKey calls through src/nes/fujinet.c, the one file that can see the
# library's own headers. The linker config is the lib's FujiNet layout with
# the CHR-ROM split so the game's pattern table (src/nes/chr.s, from the
# msdos art) sits at $1000 beside the cc65 font the runtime insists on.
ifeq ($(PLATFORM),nes)
  FUJINET_LIB = $(HOME)/Workspace/fujinet-lib-experimental
endif
CFLAGS_EXTRA_NES = -DBUILD_NES -DCUSTOM_FUJINET_CALLS -Os
NES_CFG = src/nes/nes.cfg

# CoCo: optimization + memory layout from old Makefile.coco
CFLAGS_EXTRA_COCO  += -fomit-frame-pointer -O2 -Wno-const

ifeq ($(MAKE_COCO3),COCO3)
  # CoCo 3: the 32K screen lives in MMU blocks 52-55, so the program can
  # start lower and keep clear of the $8000 graphics window.
  COCO_ORG = 1000
  CFLAGS_EXTRA_COCO  += -DCOCO3
  # --initial-s puts the stack at the top of RAM below the $8000 screen window.
  LDFLAGS_EXTRA_COCO += --org=$(COCO_ORG) --limit=7E00 --initial-s=8000
else
  COCO_ORG = 1A00
  LDFLAGS_EXTRA_COCO += --org=$(COCO_ORG) --limit=7B00
endif

# Support 'make coco3'
coco3:
	$(MAKE) coco MAKE_COCO3=COCO3

## Dragon specific flags (cmoc)
CFLAGS_EXTRA_DRAGON = \
	-Wno-assign-in-condition \
	--no-relocate \
	--intermediate \
	-DDRAGON \
	--dragon
LDFLAGS_EXTRA_DRAGON = --limit=7b00 --org=$(COCO_ORG) --dragon

# Apple II: custom HGR-aware linker config
LDFLAGS_EXTRA_APPLE2 += -C src/apple2/apple2-hgr.cfg

# NES: regenerate the pattern table from the msdos art before every build
nes/r2r::
	python3 src/nes/mkchr.py

# MS-DOS (Watcom): bundle AUTOEXEC.BAT into the disk image
msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"

adam/r2r-post::
#	Stage the DDP on the local TNFS root for loading via the TMA-3 host slot.
#	Skipped silently inside the defoogi container, where ~/tnfs is not mounted.
	-@[ -d ~/tnfs ] && cp $(EXECUTABLE) ~/tnfs/ && echo "Copied $(EXECUTABLE) to ~/tnfs/" || true

include mekkogx/toplevel-rules.mk

# NES is not in PLATFORMS (see above), so `make nes` needs spelling out
.PHONY: nes
nes: $(R2R_DIR)/nes/$(PRODUCT)

#################################################################
## CUSTOM DISTRIBUTION RECIPES                                 ##
#################################################################

# Variables for coco-dist. "FUJITZEE" is already 8 characters, so the
# per-model binaries use the shortened FUJITZE1 / FUJITZE3 names that the
# loader (support/coco/loader.c) expects.
R2R_PRODUCT = r2r/coco/$(PRODUCT)
COCO_DISK   = $(R2R_PRODUCT).dsk

# Combined CoCo 1/2 + CoCo 3 disk, with a loader that auto-detects the model.
coco-dist:
	$(MAKE) clean
	rm -rf build
	$(MAKE) coco
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)1.bin

	rm -rf build
	$(MAKE) coco3
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)3.bin

	cmoc -o $(R2R_PRODUCT).bin support/coco/loader.c

	$(RM) $(COCO_DISK)
	decb dskini $(COCO_DISK)
	echo RUNM\"$(PRODUCT_UPPER)\" > build/coco/autoexec.bas
	decb copy -t -0 build/coco/autoexec.bas $(COCO_DISK),AUTOEXEC.BAS
	decb copy -b -2 $(R2R_PRODUCT).bin  $(COCO_DISK),$(PRODUCT_UPPER).BIN
	decb copy -b -2 $(R2R_PRODUCT)1.bin $(COCO_DISK),FUJITZE1.BIN
	decb copy -b -2 $(R2R_PRODUCT)3.bin $(COCO_DISK),FUJITZE3.BIN

#################################################################
## NES TESTING                                                 ##
#################################################################

# Headless smoke test in MAME's nes driver. The MAME tree needs
# fn-nes/pico/nes/emu/apply.sh run against it once for -nes_slot fujinet to
# exist, and fujinet-pc listening on 127.0.0.1:9995 (bus over IP). The screen
# is read out of the nametable through support/nes/tilemap.lua, which
# src/nes/mkchr.py writes.
#
#   make nes-smoke                           print the screen
#   make nes-smoke EXPECT="choose a game"    and assert on it
#   make nes-smoke SCRIPT="a,wait5,a"        drive the joypad first
#                                            (a b select start up down left right waitN)
#   make nes-smoke SNAP=/tmp/fz.png          and save a screenshot
#   make nes-play                            play it in a window, with sound
#
# MAME runs throttled: unthrottled it races the live game server, whose
# clocks are wall time.
MAME_DIR    ?= $(HOME)/Workspace/mame
NES_ROM     := $(CURDIR)/r2r/nes/$(PRODUCT).nes
AT          ?= 8
EXPECT      ?=
SCRIPT      ?=
SNAP        ?=
SETTLE      ?= 8
comma       := ,
SECS        ?= $(shell echo $$(( $(AT) + $(SETTLE) + 4 + 2 * $(words $(subst $(comma), ,$(SCRIPT))) )))

.PHONY: nes-smoke nes-play

nes-smoke:
	cd $(MAME_DIR) && \
	FZ_TILEMAP=$(CURDIR)/support/nes/tilemap.lua FZ_AT=$(AT) FZ_EXPECT="$(EXPECT)" \
	FZ_SCRIPT="$(SCRIPT)" FZ_SETTLE=$(SETTLE) FZ_SNAP="$(SNAP)" \
	./mame nes -nes_slot fujinet -cart $(NES_ROM) \
	    -video none -sound none -seconds_to_run $(SECS) \
	    -autoboot_script $(CURDIR)/support/nes/smoke.lua

nes-play:
	cd $(MAME_DIR) && ./mame nes -nes_slot fujinet -cart $(NES_ROM) -window
