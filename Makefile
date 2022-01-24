CC      := /usr/bin/gcc
CFLAGS  := -std=gnu99 -Wall -pedantic

sources := $(wildcard *.c)
target  := a.out

ifdef NDEBUG
	CFLAGS += -O2 -march=native -mtune=native -DNDEBUG
else
	CFLAGS += -O0 -ggdb -DTC_DEBUG_METRICS
endif

$(target): $(sources)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(target)

.PHONY: clean
