all: toot

update:
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

AJL/ajlcurl.o: AJL/ajl.o
	make -C AJL

toot: toot.c AJL/ajlcurl.o
	cc -O -o $@ $< ajlcurl.o -lcurl -lm
