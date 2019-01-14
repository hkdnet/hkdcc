SRCS=$(wildcard *.c)

build/hkdcc: $(SRCS)
	mkdir -p build
	cd build && cmake ../ && make

fmt: $(SRCS) hkdcc.h
	clang-format -i $(SRCS) hkdcc.h

deps:
	brew install clang-format # for OSX

test: build/hkdcc
	build/hkdcc -test
	./test.sh

clean:
	rm -rf build
