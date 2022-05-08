CC  = gcc
SRC = $(wildcard *.c)
EXE = $(SRC:%.c=%)

.PHONY: all clean test-%

%: %.c
	$(CC) -o $@ $^

all: $(EXE)

clean:
	rm -f $(EXE)

test-%: %
	./$<

