all: sigd

sigd: sigd.c
	gcc -O2 -o sigd sigd.c -lwiringPi -lzmq

clean:
	rm -f sigd

install:
	install sigd /usr/local/bin/sigd
