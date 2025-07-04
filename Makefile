# Allow overriding compiler and flags from the environment which is
# common when cross compiling under environments like Yocto.
ifeq ($(origin CC),default)
CC = gcc
endif
CFLAGS ?= -std=c11 -Wall -Wextra
TARGET=grubenv

all: $(TARGET)

$(TARGET): grubenv.c version.h
	$(CC) $(CFLAGS) -o $@ grubenv.c

lint:
	$(CC) $(CFLAGS) -fsyntax-only grubenv.c

test: $(TARGET)
	./tests.sh

clean:
	rm -f $(TARGET) envtest grubfile ourfile a b
