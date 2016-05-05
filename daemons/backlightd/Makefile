all: backlightd

backlightd: backlightd.c
	gcc -O2 -o backlightd backlightd.c -lwiringPi -lpthread -lzmq

clean:
	rm -f backlightd

install:
	install backlightd /usr/local/bin/backlightd
