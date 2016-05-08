all: cmpd proxy

wicked: wicked.c
	gcc -O2 -o wicked wicked.c -lwiringPi

cmpd: cmpd.c
	gcc -O2 -o cmpd cmpd.c -lzmq -lmpdclient

proxy: proxy.c
	gcc -O2 -o proxy proxy.c -lzmq

clean:
	rm -f cmpd proxy

install-card:
	install card.py /usr/local/bin/card.py

install-gpslog:
	install gpslog.py /usr/local/bin/gpslog.py

install-cmpd: cmpd
	install cmpd /usr/local/bin/cmpd

install-proxy: proxy
	install proxy /usr/local/bin/proxy

install: install-cmpd install-proxy install-card install-gpslog
