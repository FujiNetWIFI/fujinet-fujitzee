# Makefile -- Fujitzee for the RCA Studio II: build/fz.st2 (see build.sh).
#   make                             the image
#   make ENDPOINT=http://host:port   another server
#   make run                         MAME, against a fujinet-pc on 9995 (run.sh)
#   make test                        the short scenarios on a private server
#   make soak                        a whole game, 13 rounds (about 4 minutes)
#   make shots                       the README's screenshots into build/shots
ENDPOINT ?= https://fujitzee.carr-designs.com/
SCEN     := boot join turn lobby views

all:
	ENDPOINT=$(ENDPOINT) ./build.sh

run:
	./run.sh

test:
	mkdir -p build/scr
	set -e; for s in $(SCEN); do echo "== $$s"; test/rig.sh test/scen/$$s.scen build/scr 120 | grep -v '^Warn'; done

soak:
	mkdir -p build/scr
	test/rig.sh test/scen/soak.scen build/scr 800 | grep -E 'TEXT|SNAP|DONE'

shots:
	mkdir -p build/shots
	NAME= test/rig.sh test/scen/name.scen build/shots 40
	test/rig.sh test/scen/shots.scen build/shots 700

clean:
	rm -rf build

.PHONY: all run test soak shots clean
