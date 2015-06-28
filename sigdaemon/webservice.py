#!/usr/bin/env python2.7
#
# Web service used to proxy requests from Android
#
# Access with `curl http://<server>/battery?state=[01]
# 
# State 0 = battery discharging
# State 1 = battery full

import falcon
import zmq
import logging


# ZeroMQ XSUB socket
SOCK = 'tcp://localhost:9080'


class ZeroMQ(object):
    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUB)
        self.socket.connect(SOCK)
        logging.info("Publishing events to %s" % SOCK)

    def send(self, msg):
        return self.socket.send_string(msg)


class BatteryResource(object):
    def __init__(self, zeromq):
        self.zeromq = zeromq

    def on_get(self, req, resp):
        try:
            state = req.get_param('state')
            if state == '1':
                msg = 'BATTERY FULL'
            elif state == '0':
                msg = 'BATTERY CHARGING'
            else:
                raise Exception('Required parameter "state" invalid\n')
        except Exception, e:
            resp.status = falcon.HTTP_400
            resp.body = unicode(e)
            return

        try:
            self.zeromq.send(msg)
            resp.status = falcon.HTTP_202
            resp.body = 'State has been transmitted\n'
        except Exception, e:
            resp.status = falcon.HTTP_500
            resp.body = 'Unexpected exception: %s\n' % unicode(e)


# start ZeroMQ socket
zeromq = ZeroMQ()

# start the WSGI application
app = falcon.API()
battery = BatteryResource(zeromq)
app.add_route('/battery', battery)
