bin/hkdcc: hkdcc.c
	gcc -o $@ hkdcc.c

fmt: hkdcc.c
	clang-format -i hkdcc.c

test: bin/hkdcc
	./test.sh

clean:
	rm -f hkdcc *.o *~ tmp* bin/*
