#!/usr/bin/env python2.7
#
# Proxy server for ZeroMQ events

import zmq
import logging


# ZeroMQ XSUB socket
XSUB = "tcp://0.0.0.0:9080"

# ZeroMQ XPUB socket
XPUB = "tcp://0.0.0.0:9090"


class Proxy(object):
    def __init__(self):
        logging.info("Setting up ZeroMQ sockets...")
        self.context = zmq.Context()
        self.xsub = self.context.socket(zmq.XSUB)
        self.xpub = self.context.socket(zmq.XPUB)
        self.xsub.bind(XSUB)
        self.xpub.bind(XPUB)
        logging.info("ZeroMQ XSUB listening on %s" % XSUB)
        logging.info("ZeroMQ XPUB listening on %s" % XPUB)

    def run(self):
        logging.info("Proxy server starting.")
        zmq.proxy(self.xsub, self.xpub)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    proxy = Proxy()

    try:
        proxy.run()

    except KeyboardInterrupt:
        pass
