import zmq

context = zmq.Context()

#  Socket to talk to server
socket = context.socket(zmq.SUB)
socket.connect("tcp://raspberrypi:5550")
socket.setsockopt_string(zmq.SUBSCRIBE, u"")

while True:
    message = int(socket.recv())
    print "Ignition power changed to %s" % ("ON" if message & 1 else "OFF")
