SRCS=$(wildcard *.c)

build/hkdcc: $(SRCS) dummy.o
	mkdir -p build
	cd build && cmake ../ && make

dummy.o: dummy.c

fmt: $(SRCS) hkdcc.h
	clang-format -i $(SRCS) hkdcc.h

deps:
	brew install clang-format # for OSX

test: build/hkdcc
	build/hkdcc -test
	./test.sh

clean:
	rm -rf build

compile_commands.json:
	mkdir -p build
	cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ../ && make
	cp build/compile_commands.json ./
