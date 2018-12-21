bin/hkdcc: hkdcc.c
	gcc -o $@ hkdcc.c

test: bin/hkdcc
	./test.sh

clean:
	rm -f hkdcc *.o *~ tmp* bin/*
