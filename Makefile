CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

bin/hkdcc: $(OBJS)
	gcc -o $@ $^

$(OBJS): hkdcc.h

fmt: $(SRCS) hkdcc.h
	clang-format -i $(SRCS) hkdcc.h

deps:
	brew install clang-format # for OSX

test: bin/hkdcc
	./test.sh

clean:
	rm -f hkdcc *.o *~ tmp* bin/*
