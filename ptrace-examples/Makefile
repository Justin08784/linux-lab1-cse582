CC = gcc
CFLAGS = -Wall -g
TARGET = sys mem restore test1 test2

.PHONY: all
all: $(TARGET)

sys: sys.c
	$(CC) $(CFLAGS) -o sys sys.c

mem: mem.c
	$(CC) $(CFLAGS) -o mem mem.c

restore: restore.c
	$(CC) $(CFLAGS) -o restore restore.c

test1: test1.c
	$(CC) $(CFLAGS) -o $@ $^

test2: test2.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(TARGET)

