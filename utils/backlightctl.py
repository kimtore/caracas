#!/usr/bin/env python2.7
# coding: utf-8

import time
import zmq
import argparse

SOCK = "tcp://localhost:9080"
CONNECT_TIMEOUT = 0.05

socket = None


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('value', type=int)
    args = parser.parse_args()

    # client sanitation, other values will be ignored on the receiver
    if args.value < 0:
        args.value = 0
    elif args.value > 100:
        args.value = 100

    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.connect(SOCK)

    # wait a bit before sending the messages
    time.sleep(CONNECT_TIMEOUT)

    socket.send_string("BACKLIGHT %d" % args.value)
    print 'Setting backlight intensity to %d%%' % args.value
    socket.close()
