all: sigd

sigd: sigd.c
	gcc -O2 -o sigd sigd.c -lwiringPi -lzmq

clean:
	rm sigd

install-sigd: sigd
	install sigd /usr/local/bin/sigd

install-card:
	install card.py /usr/local/bin/card.py

install: install-sigd install-card
