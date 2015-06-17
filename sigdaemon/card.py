#!/usr/bin/env python2.7

import sys
import zmq
import threading
import logging
import subprocess

import mpd

SOCK = "tcp://localhost:9090"
SHUTDOWN_SECONDS = 15
VOLUME_STEP = 1

button_mode = 'neutral'
mode_dispatched = True

pending_shutdown = False

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logging.info("Setting up ZeroMQ socket...")
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect(SOCK)
socket.setsockopt_string(zmq.SUBSCRIBE, u'')
logging.info("Listening for events from signal daemon on %s" % SOCK)
mpd_client = mpd.MPDClient()

#
# Actual implementations
#

def run_dmc(cmd):
    logging.info("Running command: %s" % ' '.join(cmd))
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    exit_code = process.returncode
    return exit_code, stderr, stdout

def shutdown():
    logging.info("Shutting down the entire system!")
    run_dmc(['/sbin/init', '0'])

def try_shutdown():
    logging.info("Checking whether system should be shut down...")
    if pending_shutdown:
        logging.info("Yes!")
        shutdown()
        return True
    logging.info("No.")
    return False

def android_toggle_screen():
    logging.info("Toggling Android screen on/off")
    run_dmc(['curl', 'http://cardroid:8765'])

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

def mpd_prev():
    status = mpd_get_status()
    logging.info("Switching to previous song")
    mpd_client.previous()

def mpd_next():
    status = mpd_get_status()
    logging.info("Switching to next song")
    mpd_client.next()

def mpd_play_or_pause():
    status = mpd_get_status()
    if status['state'] == 'stop':
        logging.info("Starting to play music")
        mpd_client.play()
    else:
        logging.info("Toggling play/pause state")
        mpd_client.pause()

def mpd_volume_shift(increment):
    status = mpd_get_status()
    shifted = int(status['volume']) + increment
    if shifted > 100:
        shifted = 100
    if shifted < 0:
        shifted = 0
    logging.info("Setting volume to %d" % shifted)
    mpd_client.setvol(shifted)

#
# Dispatcher functions
#

def dispatch_neutral_rotary_left(params):
    mpd_volume_shift(-VOLUME_STEP)

def dispatch_mode_rotary_left(params):
    mpd_prev()

def dispatch_neutral_rotary_right(params):
    mpd_volume_shift(VOLUME_STEP)

def dispatch_mode_rotary_right(params):
    mpd_next()

def dispatch_neutral_rotary_press(params):
    mpd_play_or_pause()

def dispatch_mode_rotary_press(params):
    android_toggle_screen()

def dispatch_neutral_volume_down_press(params):
    mpd_volume_shift(-VOLUME_STEP)

def dispatch_neutral_volume_up_press(params):
    mpd_volume_shift(VOLUME_STEP)

def dispatch_neutral_arrow_up_press(params):
    mpd_next()

def dispatch_neutral_arrow_down_press(params):
    mpd_prev()

def dispatch_neutral_power_on(params):
    global pending_shutdown
    pending_shutdown = False

def dispatch_neutral_power_off(params):
    global pending_shutdown
    pending_shutdown = True
    logging.info("Ignition power has been lost, shutting down in %d seconds..." % SHUTDOWN_SECONDS)
    timer = threading.Timer(SHUTDOWN_SECONDS, try_shutdown)
    timer.start()

def dispatch_mode_power_on(params):
    global pending_shutdown
    pending_shutdown = False

def dispatch_mode_power_off(params):
    """
    Shutting down the power when the MODE key is held down prevents the
    hardware from being turned off at all.
    """
    logging.info("MODE key held down when turning off power, system is NOT shutting down.")

def dispatch_neutral_mode_press(params):
    global button_mode
    global mode_dispatched
    button_mode = 'mode'
    mode_dispatched = False

def dispatch_mode_mode_depress(params):
    global button_mode
    button_mode = 'neutral'
    if not mode_dispatched:
        android_toggle_screen()

def get_dispatch_function(tokens):
    """
    Figure out which dispatcher function to use
    """
    pattern = "dispatch_%s_%s"
    base_name = '_'.join(tokens).lower()
    func_name = pattern % (button_mode, base_name)
    logging.debug("Looking for function name '%s'" % func_name)
    try:
        func = globals()[func_name]
    except KeyError:
        return None
    return func


#
# Main loop
#

if __name__ == '__main__':
    try:
        while True:
            string = socket.recv_string()
            logging.info("Received event '%s'" % string)
            tokens = string.strip().split()
            func = get_dispatch_function(tokens)
            try:
                if callable(func):
                    logging.debug("Dispatching to '%s'" % str(func.__name__))
                    func([])
                    if tokens[0] != 'MODE':
                        mode_dispatched = True
                else:
                    logging.warn("No dispatcher found for this event")
            except Exception, e:
                logging.error("Error while dispatching: %s" % unicode(e))

    except KeyboardInterrupt:
        pass
