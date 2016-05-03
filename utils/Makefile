all: wicked

wicked: wicked.c
	gcc -O2 -o wicked wicked.c -lwiringPi

install: install-gpscat install-eventgen

install-eventgen:
	install eventgen.py /usr/local/bin

install-gpscat:
	install gpscat.py /usr/local/bin

clean:
	rm -f wicked
