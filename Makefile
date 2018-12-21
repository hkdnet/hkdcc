bin/hkdcc: fmt hkdcc.c
	gcc -o $@ hkdcc.c

fmt: hkdcc.c
	clang-format -i hkdcc.c

deps:
	brew install clang-format # for OSX

test: bin/hkdcc
	./test.sh

clean:
	rm -f hkdcc *.o *~ tmp* bin/*
