CC := /usr/bin/gcc
CFLAGS := -O2 -march=native -mtune=native -Wall

sources := $(wildcard *.c)
targets := $(sources:%.c=x.%)

$(targets): x.%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(targets)

.PHONY: clean
