CC      := /usr/bin/gcc
CFLAGS  := -std=gnu11 -Wall -pedantic
CLIBS   := -lm

sources := $(wildcard *.c)
target  := a.out

ifdef NDEBUG
CFLAGS  += -O2 -march=native -mtune=native -DNDEBUG
else
CFLAGS  += -O0 -ggdb -DTC_DEBUG_METRICS
endif

app: $(target)

$(target): $(sources)
	$(CC) $(CFLAGS) $(CLIBS) -o $@ $^

demo_sources := $(wildcard demos/*.c)
demo_targets := $(demo_sources:%.c=%)

demos: clean_demos $(demo_targets)

$(demo_targets): %: %.c
	$(CC) $(CFLAGS) $(CLIBS) -I./ -DAPP_NO_MAIN -o $@ $^ $(sources)

clean:
	rm -rf $(target)
	rm -rf $(demo_targets)

clean_demos:
	rm -rf $(demo_targets)

.PHONY: app demos clean clean_demos
