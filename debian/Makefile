
# -*- sh -*-

# Written by Abraham van der Merwe <abz@blio.net>
# Last updated: 2002-03-26

CC = gcc
CFLAGS = -Wall -Os -pipe
LDFLAGS = -s

STRIP = strip
STRIPFLAGS = --strip-all --remove-section=.note --remove-section=.comment

OBJ =
SRC = $(OBJ:%.o=%.c)
PRG =

all: #$(PRG)

$(PRG): $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf tint
	rm -f *~ $(OBJ) $(PRG) *.substvars *.debhelper

.PHONY: all clean

