
#
#   Raspberry PI ZMQ server notifying Caracas with device state
#   Uses REQ and REP socket from and to tcp://*:5555
#

import zmq

context = zmq.Context()

def get_power_status():
    return { 'net': 1, 'ignition': 1 }

class Server:
    def setup_rep_socket(self):
        self.rep = context.socket(zmq.REP)
        self.rep.bind("tcp://*:5555")

    def setup_req_socket(self):
        self.req = context.socket(zmq.REQ)
        self.req.connect("tcp://raspberrypi:5555")

    def get_next_message(self):
        return self.rep.recv()

    def send_reply(self, msg):
        return self.rep.send_string(msg)

    def dispatch(self, msg):
        if msg == 'get_power_status':
            ps = get_power_status()
            code = 0
            if ps['net']:
                code |= 0b1
            if ps['ignition']:
                code |= 0b10
            return "%d" % code
        return None

    def __init__(self):
        self.setup_rep_socket()
        self.setup_req_socket()

server = Server()

while True:
    msg = server.get_next_message()
    print "Got message: %s" % msg
    payload = server.dispatch(msg)
    if not payload:
        payload = 'ERROR'
    server.send_reply(payload)
    print "Sent back: %s" % payload
