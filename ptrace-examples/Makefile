CC = gcc
CFLAGS = -Wall -g
TARGET = sys mem

.PHONY: all
all: $(TARGET)

sys: sys.c
	$(CC) $(CFLAGS) -o sys sys.c

mem: mem.c
	$(CC) $(CFLAGS) -o mem mem.c

.PHONY: clean
clean:
	rm -f $(TARGET)

