CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
TARGET = shellite

all: $(TARGET)
$(TARGET): main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm -f $(TARGET)

.PHONY: all clean