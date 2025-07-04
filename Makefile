CC=gcc
CFLAGS=-std=c11 -Wall -Wextra
TARGET=grubenv

all: $(TARGET)

$(TARGET): grubenv.c
	$(CC) $(CFLAGS) -o $@ $<

lint:
	$(CC) $(CFLAGS) -fsyntax-only grubenv.c

test: $(TARGET)
	./tests.sh

clean:
	rm -f $(TARGET) envtest grubfile ourfile a b
