#!/usr/bin/env python2.7

import sys
import zmq
import logging
import subprocess

import mpd

SOCK = "tcp://localhost:9090"
mode = 'neutral'
mpd_client = mpd.MPDClient()

# filename='/var/log/card.log', filemode='a', 
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s %(message)s')

# Socket to talk to server
logging.info("Setting up ZeroMQ socket...")
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect(SOCK)
socket.setsockopt_string(zmq.SUBSCRIBE, u'')
logging.info("Listening for events from signal daemon on %s" % SOCK)

def run_dmc(cmd):
    logging.info("Running command: %s" % ' '.join(cmd))
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    exit_code = process.returncode
    return exit_code, stderr, stdout

def shutdown():
    run_dmc(['init', '0'])

def mpd_connect():
    mpd_client.timeout = 5
    mpd_client.idletimeout = None
    mpd_client.connect('localhost', 6600)

def mpd_get_status():
    try:
        status = mpd_client.status()
    except mpd.ConnectionError:
        mpd_connect()
        status = mpd_client.status()
    return status

def play_or_pause():
    status = mpd_get_status()
    if status['state'] == 'stop':
        mpd_client.play()
    else:
        mpd_client.pause()

def volume_shift(increment):
    status = mpd_get_status()
    shifted = int(status['volume']) + increment
    if shifted > 100:
        shifted = 100
    if shifted < 0:
        shifted = 0
    mpd_client.setvol(shifted)

def dispatch_neutral_rotary_left(params):
    volume_shift(-3)

def dispatch_neutral_rotary_right(params):
    volume_shift(3)

def dispatch_neutral_rotary_press(params):
    play_or_pause()

def dispatch_neutral_power_off(params):
    shutdown()

def get_dispatch_function(tokens):
    pattern = "dispatch_%s_%s"
    base_name = '_'.join(tokens).lower()
    func_name = pattern % (mode, base_name)
    logging.debug("Looking for function name '%s'" % func_name)
    try:
        func = globals()[func_name]
    except KeyError:
        return None
    return func


if __name__ == '__main__':
    try:
        while True:
            string = socket.recv_string()
            logging.info("Received event '%s'" % string)
            tokens = string.strip().split()
            func = get_dispatch_function(tokens)
            try:
                if callable(func):
                    logging.info("Dispatching to '%s'" % str(func.__name__))
                    func([])
                else:
                    logging.warn("No dispatcher found for this event")
            except Exception, e:
                logging.error("Error while dispatching: %s" % unicode(e))

    except KeyboardInterrupt:
        pass
