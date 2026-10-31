# Makefile -- Fujitzee for the Odyssey 2: build/fujitzee.bin (build.sh).
# Standalone, like intv/ and astrocade/: not part of the root mekkogx build.
#
#   make                  the image, against the live server
#   make ENDPOINT=http://host:port/
#   make spike            the table engine's test pattern (build/spike)
#   make run              play it in a MAME window (run.sh)
#   make shot             the spike in MAME, headless, and tools/rowcheck.py
#   make rig              build for the private server (build/rigimg)
#   make test             the live scenarios against the private rig
#   make soak             an AUTOPLAY build plays a game at ai4 (4 bots)
#   make over             game over in seconds (test/endgame.py)
#   make two              an AUTOPLAY build against test/player.py at kit
#   make shots            the README's screenshots (screenshots/)

# Every target rebuilds: ENDPOINT and the flags are not files make can see.
ENDPOINT ?= https://fujitzee.carr-designs.com/
RIG_ENDPOINT = http://localhost:8091/

all:
	ENDPOINT='$(ENDPOINT)' ./build.sh

spike:
	SPIKE=1 OUT=build/spike ./build.sh

shot: spike
	O2SIM_SECONDS=20 python3 tools/o2sim.py test/scen/spike.scen build/spike/run build/spike/fujitzee.bin
	python3 tools/rowcheck.py build/spike/run rows

run: all
	./run.sh

# The rig's image has its own directory: build/fujitzee.bin stays the live
# server's, for ./run.sh (which builds it only when it's missing).
RIG_IMG = build/rigimg/fujitzee.bin

rig:
	ENDPOINT='$(RIG_ENDPOINT)' OUT=build/rigimg ./build.sh

test: rig
	test/rig.sh test/scen/boot.scen build/run/boot 50 $(RIG_IMG)
	test/rig.sh test/scen/play.scen build/run/play 90 $(RIG_IMG)
	NAME= test/rig.sh test/scen/name.scen build/run/name 25 $(RIG_IMG)
	ROOM="http://localhost:8091/?table=ai2" test/rig.sh test/scen/room.scen build/run/room 25 $(RIG_IMG)

soak:
	AUTOPLAY=1 OUT=build/auto ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	O2SIM_SHOTS=600 test/rig.sh test/scen/autoplay.scen build/run/auto 560 build/auto/fujitzee.bin

over: rig
	ENDGAME="--table ai2 --after 8" test/rig.sh test/scen/over.scen build/run/over 110 $(RIG_IMG)

two:
	AUTOPLAY=1 OUT=build/auto ENDPOINT='$(RIG_ENDPOINT)' ./build.sh
	PLAYER="--table kit --games 1" O2SIM_SHOTS=600 test/rig.sh test/scen/twohuman.scen build/run/two 170 build/auto/fujitzee.bin

shots: rig
	rm -rf build/run/shots
	NAME= ENDGAME="--table ai2 --name thom --after 50 --winners 0" test/rig.sh test/scen/shots.scen build/run/shots 150 $(RIG_IMG)
	mkdir -p screenshots
	cp build/run/shots/0*.png screenshots/

clean:
	rm -rf build

.PHONY: all spike shot run rig test soak over two shots clean
