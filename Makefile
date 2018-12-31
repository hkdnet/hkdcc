CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

bin/hkdcc: $(OBJS)
	gcc -o $@ $^

$(OBJS): hkdcc.h

fmt: $(SRCS)
	clang-format -i $(SRCS)

deps:
	brew install clang-format # for OSX

test: bin/hkdcc
	./test.sh

clean:
	rm -f hkdcc *.o *~ tmp* bin/*
