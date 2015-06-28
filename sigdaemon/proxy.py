#!/usr/bin/env python2.7
#
# Proxy server for ZeroMQ events

import zmq
import syslog


# ZeroMQ XSUB socket
XSUB = "tcp://0.0.0.0:9080"

# ZeroMQ XPUB socket
XPUB = "tcp://0.0.0.0:9090"


class Proxy(object):
    def __init__(self):
        syslog.syslog("Setting up ZeroMQ sockets...")
        self.context = zmq.Context()
        self.xsub = self.context.socket(zmq.XSUB)
        self.xpub = self.context.socket(zmq.XPUB)
        self.xsub.bind(XSUB)
        self.xpub.bind(XPUB)
        syslog.syslog("ZeroMQ XSUB listening on %s" % XSUB)
        syslog.syslog("ZeroMQ XPUB listening on %s" % XPUB)

    def run(self):
        syslog.syslog("Proxy server starting.")
        zmq.proxy(self.xsub, self.xpub)


if __name__ == '__main__':
    syslog.openlog('proxy', syslog.LOG_PID, syslog.LOG_DAEMON)
    proxy = Proxy()

    try:
        proxy.run()

    except KeyboardInterrupt:
        syslog.syslog(syslog.LOG_NOTICE, "Exiting program.")
