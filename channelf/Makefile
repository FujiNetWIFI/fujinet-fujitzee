# Fujitzee for the Fairchild Channel F.
ASL         ?= $(HOME)/asl/asl
P2BIN       ?= $(HOME)/asl/p2bin
MAME_DIR    ?= $(HOME)/Workspace/mame
FUJINET_TCP ?= 127.0.0.1:9995

SRCS := $(wildcard src/*.asm src/*.inc) tools/mkfont.py

all: build/fujitzee.bin

build/fujitzee.bin: $(SRCS) build.sh
	ASL=$(ASL) P2BIN=$(P2BIN) ./build.sh

run: all
	./run.sh

assets:
	python3 tools/mkfont.py src/font.inc

clean:
	rm -rf build src/font.inc src/endpoint.inc

.PHONY: all run assets clean
