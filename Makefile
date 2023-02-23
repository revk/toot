all: toot

OPTS=-D_GNU_SOURCE --std=gnu99 -g -Wall -funsigned-char -lpopt

update:
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

AJL/ajlcurl.o: AJL/ajl.c
	make -C AJL

toot: toot.c AJL/ajlcurl.o
	cc -O -o $@ $< AJL/ajlcurl.o -lcurl -lm -IAJL ${OPTS}
