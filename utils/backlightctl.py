#!/usr/bin/env python2.7
# coding: utf-8

import time
import zmq
import argparse

SOCK = "tcp://localhost:9080"
CONNECT_TIMEOUT = 0.05

socket = None


def get_zmq_socket():
    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.connect(SOCK)
    # wait a bit before sending the messages
    time.sleep(CONNECT_TIMEOUT)
    return socket


def set_value(a):
    socket = get_zmq_socket()
    print 'Setting backlight intensity to %d%%' % a.value
    socket.send_string("BACKLIGHT %d" % a.value)


def fade(a):
    socket = get_zmq_socket()
    print 'Fading backlight intensity from %d%% to %d%%' % (a.start, a.end)
    if a.start == a.end:
        return
    direction = 1 if a.start < a.end else -1
    x = a.start
    while True:
        x += direction
        socket.send_string("BACKLIGHT %d" % x)
        print "BACKLIGHT %d" % x
        if x == a.end:
            break


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser_set = subparsers.add_parser('set')
    parser_set.add_argument('value', type=int)
    parser_set.set_defaults(func=set_value)

    parser_fade = subparsers.add_parser('fade')
    parser_fade.add_argument('start', type=int)
    parser_fade.add_argument('end', type=int)
    parser_fade.set_defaults(func=fade)

    args = parser.parse_args()
    args.func(args)
