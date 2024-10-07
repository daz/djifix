#!/usr/bin/env make -f
# Makefile to update and build djifix

prefix=/usr/local
url="https://djifix.live555.com/djifix.c"
version=$(shell grep 'versionStr = ' djifix.c | sed 's/.*"\(.*\)".*/\1/')

.PHONY: all build clean install update commit release version

all: build

build: djifix

djifix: djifix.c
	$(CC) $(CFLAGS) -O -o djifix djifix.c

clean:
	rm djifix

install: djifix
	install -d $(prefix)/bin
	install -s -m 0755 djifix $(prefix)/bin

update:
	curl -s $(url) -o tmp.txt
	@if [ -s tmp.txt ]; then \
		mv tmp.txt djifix.c; \
	else \
		rm tmp.txt; \
	fi

commit:
	@if git diff --quiet --exit-code djifix.c; then \
		echo "No changes to commit in djifix.c."; \
	else \
		git commit djifix.c -m "Version $(version)"; \
	fi

release:
	git push origin master

version:
	@echo $(version)
