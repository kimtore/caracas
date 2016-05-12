#!/usr/bin/env python2.7

import os
import sys
import zmq
import argparse
import syslog
import subprocess32
import datetime
import time


# Touch this file to prevent shutdown by Card in any circumstance
SHUTDOWN_PREVENT_FILE = '/tmp/keepalive'

# ZeroMQ publisher socket
PUB_SOCK = "tcp://localhost:9090"

# ZeroMQ subscriber socket
SUB_SOCK = "tcp://localhost:9080"

# How many seconds to keep the system alive during loss of ignition power
POWER_TIMEOUT = 900

# Delta MPD volume each time a volume change event is triggered
VOLUME_STEP = 2

# Delta backlight intensity each time a backlight change event is triggered
BACKLIGHT_STEP = 5

# Repeat interval for repeatable commands, in milliseconds
TICK = 100

# Subprocess timeout in seconds
SUBPROCESS_TIMEOUT = 3


def repeatable(func):
    """
    Decorator that specifies that an event should be repeated at the next tick
    """
    def func_wrapper(class_):
        func(class_)
        return lambda: func(class_)
    return func_wrapper


class System(object):
    """
    Trigger system events such as run, shutdown, etc.
    """
    def run(self, cmd):
        syslog.syslog("Running shell command: %s" % ' '.join(cmd))
        try:
            process = subprocess32.Popen(cmd, stdout=subprocess32.PIPE, stderr=subprocess32.PIPE)
            stdout, stderr = process.communicate(timeout=SUBPROCESS_TIMEOUT)
        except Exception, e:
            syslog.syslog("Failed opening subprocess with shell command: %s" % unicode(e))
            return -1, [], []
        syslog.syslog("Shell command finished with return code %d" % process.returncode)
        return process.returncode, stderr, stdout

    def shutdown(self):
        self.run(['/bin/systemctl', 'halt'])


class Dispatcher(object):
    """
    Map button events to correct actions
    """
    def __init__(self, system):
        self.system = system
        self.power = True  # assume power on at boot
        self.power_on_time = None
        self.hibernated = False
        self.display_intensity = 100
        self.target_display_intensity = 100
        self.set_power_on_time()
        self.button_mode = 'neutral'
        self.mode_dispatched = True
        self.is_shutting_down = False
        self.init_zeromq()

    def init_zeromq(self):
        syslog.syslog("Setting up ZeroMQ publisher socket...")
        self.context = zmq.Context()
        self.publisher = self.context.socket(zmq.PUB)
        self.publisher.connect(SUB_SOCK)
        syslog.syslog("Publishing events to %s" % SUB_SOCK)

    def set_power_on_time(self):
        self.power_on_time = datetime.datetime.now()
        self.is_shutting_down = False

    def can_shutdown(self):
        if self.power or self.is_shutting_down:
            return False
        if self.is_keepalive():
            return False
        return True

    def needs_shutdown(self):
        if not self.can_shutdown():
            return False
        return self.shutdown_timed_out()

    def needs_hibernation(self):
        if self.hibernated:
            return False
        if self.power or self.is_shutting_down:
            return False
        return True

    def needs_thaw(self):
        return self.hibernated and self.power and not self.is_shutting_down

    def shutdown_timed_out(self):
        delta = datetime.datetime.now() - self.power_on_time
        return delta.total_seconds() > POWER_TIMEOUT

    def shutdown(self):
        self.screen_off()
        self.publisher.send_string('MPD PAUSE')
        syslog.syslog("Shutting down the entire system!")
        self.is_shutting_down = True
        self.system.shutdown()

    def boot(self):
        syslog.syslog("System booted, turning on music.")
        self.publisher.send_string('MPD UNPAUSE')

    def is_keepalive(self):
        return os.path.exists(SHUTDOWN_PREVENT_FILE)

    def fade_screen(self, target):
        if target < 0:
            target = 0
        elif target > 100:
            target = 100
        if target == self.display_intensity:
            return
        if target != 0:
            self.target_display_intensity = target
        syslog.syslog('Fading display intensity to %d.' % target)
        sign = 1 if target > self.display_intensity else -1
        for intensity in xrange(self.display_intensity, target + sign, sign):
            self.publisher.send_string('BACKLIGHT %d' % intensity)
        self.display_intensity = target

    def step_screen(self, step):
        target = self.display_intensity + step
        self.fade_screen(target)

    def screen_on(self):
        self.fade_screen(self.target_display_intensity)

    def screen_off(self):
        self.fade_screen(0)

    def screen_toggle(self):
        if self.display_intensity == 0:
            self.screen_on()
        else:
            self.screen_off()

    def hibernate(self):
        syslog.syslog('Putting system in hibernation mode.')
        self.publisher.send_string('MPD PAUSE')
        self.screen_off()
        self.hibernated = True

    def thaw(self):
        syslog.syslog('Restoring system from hibernation mode.')
        self.publisher.send_string('MPD UNPAUSE')
        self.screen_on()
        self.hibernated = False

    #
    # Rotary events
    #
    def neutral_rotary_left(self):
        self.publisher.send_string('MPD VOLUME STEP %d' % -VOLUME_STEP)

    def mode_rotary_left(self):
        self.step_screen(-BACKLIGHT_STEP)

    def neutral_rotary_right(self):
        self.publisher.send_string('MPD VOLUME STEP %d' % VOLUME_STEP)

    def mode_rotary_right(self):
        self.step_screen(BACKLIGHT_STEP)

    def neutral_rotary_press(self):
        self.publisher.send_string('MPD PLAY OR PAUSE')

    #
    # Steering wheel up/down and volumes
    #
    @repeatable
    def neutral_volume_down_press(self):
        self.publisher.send_string('MPD VOLUME STEP %d' % -VOLUME_STEP)

    @repeatable
    def neutral_volume_up_press(self):
        self.publisher.send_string('MPD VOLUME STEP %d' % VOLUME_STEP)

    def neutral_arrow_up_press(self):
        self.publisher.send_string('MPD NEXT')

    def neutral_arrow_down_press(self):
        self.publisher.send_string('MPD PREV')

    def mode_volume_down_press(self):
        self.publisher.send_string('MPD PREV ALBUM')

    def mode_volume_up_press(self):
        self.publisher.send_string('MPD NEXT ALBUM')

    def mode_arrow_up_press(self):
        self.publisher.send_string('MPD NEXT ARTIST')

    def mode_arrow_down_press(self):
        self.publisher.send_string('MPD PREV ARTIST')

    #
    # Ignition power events
    #
    def neutral_power_on(self):
        syslog.syslog("Ignition power has been restored, system will remain active.")
        self.power = True
        self.set_power_on_time()

    def neutral_power_off(self):
        syslog.syslog("Ignition power has been lost!")
        self.power = False
        self.set_power_on_time()
        if self.can_shutdown():
            syslog.syslog("Shutting down in %d seconds unless power is restored..." % POWER_TIMEOUT)
        else:
            syslog.syslog("Shutdown is temporarily disabled because internal state prevents a shutdown.")

    def mode_power_on(self):
        return self.neutral_power_on()

    def mode_power_off(self):
        """
        Shutting down the power when the MODE key is held down prevents the
        hardware from being turned off at all.
        """
        syslog.syslog("Ignition power has been lost while MODE key is held down; will behave as power is still on.")

    def neutral_mode_press(self):
        self.button_mode = 'mode'

    def mode_mode_depress(self):
        self.button_mode = 'neutral'
        if not self.mode_dispatched:
            syslog.syslog('Driver requested screen toggle')
            self.screen_toggle()

    #
    # Dispatch message arrays to functions
    #
    def dispatch(self, *args):
        ret = None
        func = self.get_dispatch_function(*args)

        set_dispatched = (self.button_mode == 'mode' and args[0] != 'mode')
        if callable(func):
            syslog.syslog(syslog.LOG_DEBUG, "Dispatching to '%s'" % str(func.__name__))
            ret = func()
        else:
            syslog.syslog(syslog.LOG_WARNING, "No dispatcher found for this event")

        self.mode_dispatched = set_dispatched
        syslog.syslog(syslog.LOG_DEBUG, "Setting mode_dispatched variable to %s" % set_dispatched)

        return ret

    def get_dispatch_function(self, *args):
        """
        Figure out which dispatcher function to use
        """
        pattern = "%s_%s"
        base_name = '_'.join(args)
        func_name = pattern % (self.button_mode, base_name)
        syslog.syslog(syslog.LOG_DEBUG, "Looking for function name 'Dispatcher.%s'" % func_name)
        func = getattr(self, func_name, None)
        return func


class Card(object):
    """
    Main loop and ZeroMQ communications
    """
    def __init__(self, dispatcher):
        self.dispatcher = dispatcher
        self.repeat_func = None

    def setup_zeromq(self):
        syslog.syslog("Setting up ZeroMQ subscriber socket...")
        self.context = zmq.Context()
        self.subscriber = self.context.socket(zmq.SUB)
        self.subscriber.connect(PUB_SOCK)
        self.subscriber.setsockopt_string(zmq.SUBSCRIBE, u'POWER')
        self.subscriber.setsockopt_string(zmq.SUBSCRIBE, u'MODE')
        self.subscriber.setsockopt_string(zmq.SUBSCRIBE, u'ARROW')
        self.subscriber.setsockopt_string(zmq.SUBSCRIBE, u'VOLUME')
        self.subscriber.setsockopt_string(zmq.SUBSCRIBE, u'ROTARY')
        self.poller = zmq.Poller()
        self.poller.register(self.subscriber, zmq.POLLIN)
        syslog.syslog("Listening for events on %s" % PUB_SOCK)

    def process_zeromq_message(self):
        string = self.subscriber.recv_string()
        syslog.syslog("Received event: %s" % string)
        tokens = string.strip().lower().split()
        self.repeat_func = self.dispatcher.dispatch(*tokens)

    def run_tick(self):
        if self.dispatcher.needs_shutdown():
            self.dispatcher.shutdown()
        if self.dispatcher.needs_hibernation():
            self.dispatcher.hibernate()
        if self.dispatcher.needs_thaw():
            self.dispatcher.thaw()
        if callable(self.repeat_func):
            syslog.syslog("Repeating last dispatched function")
            self.repeat_func()

    def run(self):
        while True:
            events = dict(self.poller.poll(TICK))
            if events:
                self.process_zeromq_message()
            else:
                self.run_tick()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', help='Enable debug output', action='store_true')
    args = parser.parse_args()

    syslog.openlog('card', syslog.LOG_PID, syslog.LOG_DAEMON)

    if not args.debug:
        syslog.setlogmask(syslog.LOG_UPTO(syslog.LOG_INFO))

    system = System()
    dispatcher = Dispatcher(system)
    time.sleep(0.2)  # wait for the ZeroMQ socket to establish a connection
    dispatcher.boot()

    card = Card(dispatcher)
    card.setup_zeromq()

    try:
        card.run()

    except KeyboardInterrupt:
        syslog.syslog(syslog.LOG_NOTICE, "Exiting program.")

    except Exception, e:
        syslog.syslog(syslog.LOG_EMERG, "Uncaught exception: %s" % unicode(e))
