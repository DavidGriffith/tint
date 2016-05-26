
# -*- sh -*-

#  Copyright (c) Abraham vd Merwe <abz@blio.net>
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the author nor the names of other contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#CROSS = arm-linux-

bindir = $(DESTDIR)/usr/games
mandir = $(DESTDIR)/usr/share/man
localstatedir = $(DESTDIR)/var/games

ifeq ($(CC),)
CC = gcc
else
ifeq ($(CC),colorgcc)
	ifneq ($(CROSS),)
	CC = gcc
	endif
endif
endif

CFLAGS = -Wall -Os -pipe
CPPFLAGS = -DSCOREFILE=\"$(localstatedir)/$(PRG).scores\" #-DUSE_RAND
LDFLAGS = -s
LDLIBS = -lncurses

STRIP = strip
STRIPFLAGS = --strip-all --remove-section=.note --remove-section=.comment

INSTALL = install

OBJ = engine.o utils.o io.o tint.o
SRC = $(OBJ:%.o=%.c)
PRG = tint

       ########### NOTHING TO EDIT BELOW THIS ###########

.PHONY: all clean do-it-all depend with-depends without-depends debian postinst

all: do-it-all postinst

ifeq (.depends,$(wildcard .depends))
include .depends
do-it-all: with-depends
else
do-it-all: without-depends
endif

without-depends: depend with-depends

depend:
	rm -f .depends
	set -e; for F in $(SRC); do $(CC) -MM $(CFLAGS) $(CPPFLAGS) $$F >> .depends; done

with-depends: $(PRG)

$(PRG): $(OBJ)
	$(CROSS)$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	$(CROSS)$(STRIP) $(STRIPFLAGS) $@

ifneq ($(DESTDIR),)
install: $(PRG)
	$(INSTALL) -d $(bindir) $(mandir) $(localstatedir)
	$(INSTALL) -s -g games -o root -m 2755 $(PRG) $(bindir)
	$(INSTALL) -g games -o root -m 0644 $(PRG).6 $(mandir)/man6
#	cp tint.scores $(localstatedir)/$(PRG).scores
#	chown root.games $(localstatedir)/$(PRG).scores
#	chmod 0664 $(localstatedir)/$(PRG).scores

uninstall:
	rm -f $(bindir)/$(PRG) $(mandir)/man6/$(PRG).6 $(localstatedir)/$(PRG).scores
endif

postinst:
	$(MAKE) -C debian

debian:
	dpkg-buildpackage -rfakeroot -k2B555AEE

clean:
	rm -f .depends *~ $(OBJ) $(PRG) {configure,build}-stamp gmon.out a.out
	rm -rf debian/$(PRG)
	rm -f debian/*.{debhelper,substvars} debian/files debian/*~

distclean: clean
	$(MAKE) -C debian clean

