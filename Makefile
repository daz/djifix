#!/usr/bin/env make -f
# Makefile to update and build djifix

prefix=/usr/local
url="http://djifix.live555.com/djifix.c"
version=$(shell cat djifix.c | grep "versionStr = " | sed 's/.*"\(.*\)".*/\1/')

all: build

build: djifix

djifix:
	$(CC) $(CFLAGS) -O -o djifix djifix.c

clean:
	/bin/rm -f djifix

install:
	/bin/mkdir -p $(prefix)/bin
	/usr/bin/install -s -m 0755 djifix $(prefix)/bin

update:
	curl -s $(url) > djifix.c

commit:
	git commit -a -m "Version $(version)"

release:
	git push origin master
