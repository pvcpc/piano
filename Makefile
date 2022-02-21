CC      := /usr/bin/gcc
CFLAGS  := -std=gnu11 -Wall -pedantic
CLIBS   := -lm

sources := $(wildcard *.c)
target  := a.out

ifdef NDEBUG
CFLAGS  += -O2 -march=native -mtune=native -DNDEBUG
else
CFLAGS  += -O0 -ggdb
endif

$(target): $(sources)
	$(CC) $(CFLAGS) $(CLIBS) -o $@ $^

# @SECTION(clean)
clean:
	rm -rf $(target)
	rm -rf $(demo_targets)

.PHONY: clean
