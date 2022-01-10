CC      := /usr/bin/gcc
CFLAGS  :=

sources := $(wildcard *.cpp)
target  := a.out

ifdef NDEBUG
	CFLAGS += -O2 -march=native -mtune=native -DNDEBUG
else
	CFLAGS += -O0 -ggdb
endif

$(target): $(sources)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(target)

.PHONY: clean
