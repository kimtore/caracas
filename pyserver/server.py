#
# Raspberry PI ZMQ server telling ignition power state
#

import zmq
import threading
import logging
import RPi.GPIO as GPIO

class GPIOReader:
    PIN_IGNITION = 12

    def __init__(self):
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(self.PIN_IGNITION, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    def wait(self):
        GPIO.wait_for_edge(self.PIN_IGNITION, GPIO.BOTH)

    def get_power_status(self):
        s = {}
        s['ignition'] = GPIO.input(self.PIN_IGNITION) == 0
        return s

    def get_power_status_int(self):
        status = self.get_power_status()
        code = 0
        if status['ignition']:
            code |= 0b1
        return code

class Sock:
    def send(self, msg):
        logging.info("ZMQ %s send: %s" % (str(self.__class__).split('.')[-1], msg))
        self.socket.send_string("%s" % msg)

class Publisher(Sock):
    def __init__(self, context):
        self.socket = context.socket(zmq.PUB)
        self.socket.bind("tcp://*:5550")

class Replier(Sock):
    def __init__(self, context):
        self.socket = context.socket(zmq.REP)
        self.socket.bind("tcp://*:5551")

    def get_message(self):
        return self.socket.recv()

class Server:
    def __init__(self, publisher, replier, gpio):
        self.publisher = publisher
        self.replier = replier
        self.gpio = gpio
        self.shutdown = False

    def emit(self):
        self.publisher.send(self.gpio.get_power_status_int())

    def gpio_thread(self):
        logging.info("Start GPIO thread")
        while not self.shutdown:
            self.publisher.send(self.gpio.get_power_status_int())
            self.gpio.wait()
        logging.info("Shutdown GPIO thread")

    def replier_thread(self):
        logging.info("Start replier thread")
        while not self.shutdown:
            self.replier.get_message()
            self.replier.send(self.gpio.get_power_status_int())
        logging.info("Shutdown replier thread")

    def main(self):
        logging.info("Start main thread")
        self.threads = [
                threading.Thread(target=self.gpio_thread),
                threading.Thread(target=self.replier_thread)
                ]
        [t.start() for t in self.threads]
        logging.info("Shutdown main thread")

if __name__ == '__main__':
    FORMAT = '%(asctime)-15s %(levelname)s %(message)s'
    logging.basicConfig(format=FORMAT, level=logging.DEBUG)

    context = zmq.Context()
    publisher = Publisher(context)
    replier = Replier(context)
    gpio = GPIOReader()
    server = Server(publisher, replier, gpio)

    try:
        server.main()
    except KeyboardInterrupt:
        logging.info("Shutdown")
        server.shutdown = True
        [t.exit() for t in server.threads]
