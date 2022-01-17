CC      := /usr/bin/gcc
CFLAGS  := -Wall -Werror -pedantic
CFLAGS  += -DTM_GLOBAL_WRITE_BUF_SIZE=1

sources := $(wildcard *.c)
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
