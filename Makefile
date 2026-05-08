PRODUCT = fujitzee
PLATFORMS = atari apple2 coco msdos

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# src/platform-specific is needed by everyone (#include "platform-specific/foo.h")
INCLUDE_DIRS = src/platform-specific

# src/include holds wrappers (stdint.h, conio.h ...) for non-cc65 toolchains.
# cc65 ships its own real versions, so adding it globally would shadow them.
EXTRA_INCLUDE_COCO  = src/include

# Default fujinet-lib version. msdos requires fujinet-lib-experimental for
# RS232 support; override at build time so it doesn't drag the experimental
# branch into other platforms.
FUJINET_LIB = 4.10.0
ifeq ($(PLATFORM),msdos)
  FUJINET_LIB = https://github.com/FozzTexx/fujinet-lib-experimental.git
endif

# CoCo: optimization + memory layout from old Makefile.coco
CFLAGS_EXTRA_COCO  += -fomit-frame-pointer -O2 -Wno-const
LDFLAGS_EXTRA_COCO += --org=1A00 --limit=7B00

# Apple II: custom HGR-aware linker config
LDFLAGS_EXTRA_APPLE2 += -C src/apple2/apple2-hgr.cfg

# MS-DOS (Watcom): bundle AUTOEXEC.BAT into the disk image
msdos/disk-post::
	mcopy -t -i $(DISK) src/msdos/AUTOEXEC.BAT "::AUTOEXEC.BAT"

include mekkogx/toplevel-rules.mk
