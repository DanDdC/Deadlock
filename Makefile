CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -O2
TARGET = banker

all: $(TARGET)

$(TARGET): banker.c
	$(CC) $(CFLAGS) -o $(TARGET) banker.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
