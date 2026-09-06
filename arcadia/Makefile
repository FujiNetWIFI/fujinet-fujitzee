all:
	./build.sh

run: all
	./run.sh

assets:
	python3 tools/mkudc.py

clean:
	rm -rf build

.PHONY: all run assets clean
