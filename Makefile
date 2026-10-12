CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -pthread
SRCS    := spsc_queue.c main.c
HDRS    := spsc_queue.h

.PHONY: all run tsan clean

all: spsc spsc_tsan

spsc: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -O2 $(SRCS) -o $@

spsc_tsan: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -O1 -g -fsanitize=thread $(SRCS) -o $@

run: spsc
	./spsc

tsan: spsc_tsan
	./spsc_tsan

clean:
	rm -f spsc spsc_tsan