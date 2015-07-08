all: sigd cmpd

sigd: sigd.c
	gcc -O2 -o sigd sigd.c -lwiringPi -lzmq

wicked: wicked.c
	gcc -O2 -o wicked wicked.c -lwiringPi

cmpd: cmpd.c
	gcc -O2 -o cmpd cmpd.c -lzmq -lmpdclient

clean:
	rm sigd cmpd

install-sigd: sigd
	install sigd /usr/local/bin/sigd

install-cmpd: cmpd
	install cmpd /usr/local/bin/cmpd

install-card:
	install card.py /usr/local/bin/card.py

install-proxy:
	install proxy.py /usr/local/bin/proxy.py

install-webservice:
	install webservice.py /usr/local/bin/webservice.py

install: install-sigd install-cmpd install-card install-proxy install-webservice
