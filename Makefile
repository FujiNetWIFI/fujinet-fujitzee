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

# Sega Master System (z88dk +sms, FujiNet mailbox cartridge). Not in
# PLATFORMS: the sms bus exists only on the add-sms branch of
# fujinet-lib-experimental, checked out as the worktree ~/Workspace/fnlib-sms,
# so it builds against that:  make sms   (or FUJINET_LIB=<path>).
# src/sms is guarded by BUILD_SMS, and CUSTOM_FUJINET_CALLS routes the network
# and AppKey calls through src/sms/fujinet.c, as on the NES. The image is 48K:
# the code in the first 32K, ending below the "FUJI" claim that
# mekkogx/sms-romstamp.py stamps at $7FDC, and the MS-DOS art as 4bpp tiles in
# the third bank (src/sms/tiles.asm, SECTION BANK_02), which appmake appends.
ifeq ($(PLATFORM),sms)
  FUJINET_LIB = $(HOME)/Workspace/fnlib-sms
  # z88dk from its checkout when it is not on PATH
  Z88DK ?= $(HOME)/Workspace/z88dk
  ifeq ($(shell command -v zcc),)
    export PATH := $(Z88DK)/bin:$(PATH)
    export ZCCCFG := $(Z88DK)/lib/config
  endif
endif
CFLAGS_EXTRA_SMS = -DBUILD_SMS -DCUSTOM_FUJINET_CALLS -O3
# 8K of RAM at $C000, stack from $DFF0 down; -m leaves the map beside the
# image so BSS and the code's end ($7FD8 is the limit) can be read off. No
# stdio: the game drives the VDP itself (src/sms/graphics.c).
LDFLAGS_EXTRA_SMS += -m \
  -pragma-define:CRT_ENABLE_STDIO=0 \
  -pragma-define:CLIB_FOPEN_MAX=0 \
  -pragma-define:CLIB_EXIT_STACK_SIZE=0

# Heathkit H89 (z88dk +cpm / +hdos, the H89 FujiNet card's 82C55 at 0xD0).
# Not in PLATFORMS: the h89 bus is fujinet-lib's h89-support branch, checked
# out at ~/Workspace/fujinet-lib with r2r/h89-{cpm,hdos} prebuilt. One tree,
# two programs: both share src/h89 through the combos below, guarded by
# __H89__ (which both platform .mk files define; the lib keys on it too). The
# game draws on the H19 terminal in 80 columns with its graphics characters
# and escape codes, and CUSTOM_FUJINET_CALLS routes the network and AppKey
# calls through src/h89/fujinet.c, which sends the screen first. Each r2r-post
# packs the program onto a 100K H17 disk:
#   make h89-hdos   r2r/h89-hdos/fujitzee.h8d: MOUNT SY1: then RUN SY1:FUJITZEE.ABS
#   make h89-cpm    r2r/h89-cpm/fujitzee.h8d:  B:FUJITZEE
PLATFORM_COMBOS += h89-cpm+=h89 h89-hdos+=h89
ifneq ($(filter h89-cpm h89-hdos,$(PLATFORM)),)
  FUJINET_LIB = $(HOME)/Workspace/fujinet-lib
  # z88dk from its checkout when it is not on PATH
  Z88DK ?= $(HOME)/Workspace/z88dk
  ifeq ($(shell command -v zcc),)
    export PATH := $(Z88DK)/bin:$(PATH)
    export ZCCCFG := $(Z88DK)/lib/config
  endif
endif
CFLAGS_EXTRA_H89-CPM = -O3 -DCUSTOM_FUJINET_CALLS
CFLAGS_EXTRA_H89-HDOS = -O3 -DCUSTOM_FUJINET_CALLS
# No stdio or file handles: the game talks to the console itself
# (src/h89/console.c). -m leaves the map beside the program, so the end of
# BSS can be checked against the top of memory.
H89_LDFLAGS = -m \
  -pragma-define:CRT_ENABLE_STDIO=0 \
  -pragma-define:CLIB_FOPEN_MAX=0 \
  -pragma-define:CLIB_OPEN_MAX=0 \
  -pragma-define:CLIB_EXIT_STACK_SIZE=0 \
  -pragma-define:CRT_ENABLE_COMMANDLINE=0
LDFLAGS_EXTRA_H89-CPM = $(H89_LDFLAGS)
LDFLAGS_EXTRA_H89-HDOS = $(H89_LDFLAGS)
# z88dk's +hdos loads programs at $4480, with the stack below. The game and
# its three 80x25 screen buffers then need a 64K H89: HDOS 2.0 there keeps
# itself above $EDB0 (S.SYSM, at $20D0, reads $EDB1); with 48K it is $ADB1,
# too low. r2r-post holds the program below H89_HDOS_TOP.
H89_HDOS_TOP ?= 0xEDB0
H89_APPMAKE ?= z88dk-appmake

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

# SMS: regenerate the tile set from the msdos art before every build, and
# hold the image to the Sega mapper after it (support/sms/mapcheck.py)
sms/r2r::
	python3 src/sms/mktiles.py

sms/r2r-post::
	python3 support/sms/mapcheck.py $(EXECUTABLE)

# MS-DOS (Watcom): bundle AUTOEXEC.BAT into the disk image
msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"

# H89: pack the program onto a 100K H17 data disk.
# HDOS: appmake +hdos wants the bare program (the .abs less its 8-byte
# header: FF 00, load, length, entry) and the load address, and names the
# file after -o -- hence the cd. The program must end below HDOS's top of
# user memory (H89_HDOS_TOP), which the map's end of BSS says.
h89-hdos/r2r-post::
	@end=$$(awk '$$1=="__BSS_END_tail" {print $$3}' $(R2R_PD)/$(PRODUCT).map | tr -d '$$'); \
	  [ -n "$$end" ] || { echo "h89-hdos: no __BSS_END_tail in the map" >&2; exit 1; }; \
	  echo "h89-hdos: program ends at \$$$$end (top of memory $(H89_HDOS_TOP))"; \
	  [ $$((0x$$end)) -le $$(($(H89_HDOS_TOP))) ] || { echo "h89-hdos: too big for HDOS in 64K" >&2; exit 1; }
	tail -c +9 $(EXECUTABLE) > $(R2R_PD)/$(PRODUCT).bin
	cd $(R2R_PD) && $(H89_APPMAKE) +hdos -b $(PRODUCT).bin \
	  --org $$(od -An -tu2 -j2 -N2 $(PRODUCT).abs | tr -d ' ') -o $(PRODUCT_UPPER)
	rm -f $(R2R_PD)/$(PRODUCT).bin
	@echo "$(R2R_PD)/$(PRODUCT).h8d: MOUNT SY1: then RUN SY1:$(PRODUCT_UPPER).ABS"

# CP/M: the hz89 format (H17, 40 tracks, 3 boot tracks), then mark the empty
# directory entries appmake leaves zeroed as erased (support/h89).
h89-cpm/r2r-post::
	cd $(R2R_PD) && $(H89_APPMAKE) +cpmdisk -f hz89 --container raw --no-com-file \
	  -b $(PRODUCT).com -o $(PRODUCT).h8d
	python3 -I support/h89/cpm-dir-fixup.py $(R2R_PD)/$(PRODUCT).h8d
	@echo "$(R2R_PD)/$(PRODUCT).h8d: B:$(PRODUCT_UPPER)"

adam/r2r-post::
#	Stage the DDP on the local TNFS root for loading via the TMA-3 host slot.
#	Skipped silently inside the defoogi container, where ~/tnfs is not mounted.
	-@[ -d ~/tnfs ] && cp $(EXECUTABLE) ~/tnfs/ && echo "Copied $(EXECUTABLE) to ~/tnfs/" || true

include mekkogx/toplevel-rules.mk

# NES, SMS and the H89 are not in PLATFORMS (see above), so `make nes`,
# `make sms`, `make h89-cpm` and `make h89-hdos` need spelling out
.PHONY: nes sms h89-cpm h89-hdos
nes: $(R2R_DIR)/nes/$(PRODUCT)
sms: $(R2R_DIR)/sms/$(PRODUCT)
h89-cpm: $(R2R_DIR)/h89-cpm/$(PRODUCT)
h89-hdos: $(R2R_DIR)/h89-hdos/$(PRODUCT)

# H89: headless smoke test in FujiNet Go H89's session runner
# (~/Workspace/fujinet-go-h89-desktop, built), booting 64K CP/M or HDOS 2.0
# with the game's disk in drive 2, against the live server. --screen loses
# reverse video; SHOT saves the screen as a .ppm. The runner's --type drops
# control characters, so ESC is --key f1, which the game takes as ESC. See
# support/h89/smoke.sh.
#   make h89-smoke                                   CP/M, the first screen
#   make h89-smoke H89_OS=hdos                       the same under HDOS
#   make h89-smoke EXPECT="CHOOSE" STEPS="--wait 6000" SHOT=/tmp/x.ppm
H89_OS ?= cpm
.PHONY: h89-smoke
h89-smoke:
	PROGRAM=$(PRODUCT_UPPER) support/h89/smoke.sh $(H89_OS) \
	  $(CURDIR)/r2r/h89-$(H89_OS)/$(PRODUCT).h8d $(STEPS)

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

#################################################################
## SMS TESTING                                                 ##
#################################################################

# Headless smoke test in MAME's sms1 driver, the same shape as nes-smoke. The
# MAME tree needs fn-sms/pico/sms/emu/apply.sh run against it once for -slot
# fujinet to exist, and fujinet-pc listening on 127.0.0.1:9995 (bus over IP).
# A claimed image over 32K boots straight into the cartridge's APP mode. The
# screen is read out of the name table through support/sms/tilemap.lua, which
# src/sms/mktiles.py writes.
#
#   make sms-smoke                             print the screen
#   make sms-smoke EXPECT="choose a game"      and assert on it
#   make sms-smoke SCRIPT="b1,wait5,pause"     drive the joypad first
#                                              (b1 b2 pause up down left right,
#                                               b2+X chords, waitN, shot, snap)
#   make sms-smoke SNAP=/tmp/fz.png            and save a screenshot
#   make sms-play                              play it in a window, with sound
#
# The BIOS's logo takes the first few seconds, hence the longer settle. MAME
# runs throttled, as for the NES: the live game server's clocks are wall time.
SMS_ROM     := $(CURDIR)/r2r/sms/$(PRODUCT).sms
SMS_SETTLE  ?= 16
SMS_SECS    ?= $(shell echo $$(( $(AT) + $(SMS_SETTLE) + 4 + 2 * $(words $(subst $(comma), ,$(SCRIPT))) )))

.PHONY: sms-smoke sms-play

sms-smoke:
	cd $(MAME_DIR) && \
	FZ_TILEMAP=$(CURDIR)/support/sms/tilemap.lua FZ_AT=$(AT) FZ_EXPECT="$(EXPECT)" \
	FZ_SCRIPT="$(SCRIPT)" FZ_SETTLE=$(SMS_SETTLE) FZ_SNAP="$(SNAP)" \
	./mame sms1 -slot fujinet -cart $(SMS_ROM) \
	    -video none -sound none -seconds_to_run $(SMS_SECS) \
	    -autoboot_script $(CURDIR)/support/sms/smoke.lua

sms-play:
	cd $(MAME_DIR) && ./mame sms1 -slot fujinet -cart $(SMS_ROM) -window
