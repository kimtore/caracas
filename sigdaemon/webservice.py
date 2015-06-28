#!/usr/bin/env python2.7
#
# Web service used to proxy requests from Android
#
# Access with `curl http://<server>/battery?level=[0..100]
# 
# Level 0..99 = battery charging
# Level 100 = battery full

import falcon
import zmq
import syslog


# ZeroMQ XSUB socket
SOCK = 'tcp://localhost:9080'


def parse_bool(s):
    return s == 'on'


class ZeroMQ(object):
    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.connect(SOCK)
        syslog.syslog("Publishing ZeroMQ events to %s" % SOCK)

    def send(self, msg):
        syslog.syslog("Publishing event: %s" % msg)
        return self.socket.send_string(msg)


class Base(object):
    def __init__(self, zeromq):
        self.zeromq = zeromq


class BatteryResource(Base):
    def on_get(self, req, resp):
        syslog.syslog("Received request from %s: %s" % (req.env['REMOTE_ADDR'], req.uri))
        try:
            lev = int(req.get_param('level'))
            if lev == 100:
                msg = 'BATTERY FULL'
            else:
                msg = 'BATTERY CHARGING'
        except Exception, e:
            resp.status = falcon.HTTP_400
            resp.body = 'Required parameter "level" invalid: %s\n' % unicode(e)
            return

        try:
            self.zeromq.send(msg)
            resp.status = falcon.HTTP_202
            resp.body = 'State has been transmitted\n'
        except Exception, e:
            resp.status = falcon.HTTP_500
            resp.body = 'Unexpected exception: %s\n' % unicode(e)


class ScreenResource(Base):
    def on_get(self, req, resp):
        syslog.syslog("Received request from %s: %s" % (req.env['REMOTE_ADDR'], req.uri))
        try:
            state = parse_bool(req.get_param('state'))
            if state:
                msg = 'SCREEN ON'
            else:
                msg = 'SCREEN OFF'
        except Exception, e:
            resp.status = falcon.HTTP_400
            resp.body = 'Required parameter "state" invalid: %s\n' % unicode(e)
            return

        try:
            self.zeromq.send(msg)
            resp.status = falcon.HTTP_202
            resp.body = 'State has been transmitted\n'
        except Exception, e:
            resp.status = falcon.HTTP_500
            resp.body = 'Unexpected exception: %s\n' % unicode(e)


# Open syslog
syslog.openlog('webservice', syslog.LOG_PID, syslog.LOG_DAEMON)
syslog.syslog('HTTP REST API is starting.')

# start ZeroMQ socket
zeromq = ZeroMQ()

# start the WSGI application
application = falcon.API()
battery = BatteryResource(zeromq)
screen = ScreenResource(zeromq)
application.add_route('/battery', battery)
application.add_route('/screen', screen)
syslog.syslog('Ready to receive requests.')
