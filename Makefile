CC     = gcc
CFLAGS = -Werror #-std=c11
LIBS   = -pthread

TARGET = c-tui

SRC = $(wildcard *.c)
OBJ = $(SRC:*.c=*.o)

.PHONY: all run clean

all: $(TARGET)

*.o: *.c
	$(CC) -c -o $@ $^ $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run: $(TARGET)
	./$<

clean:
	rm -rf *.o $(TARGET)
