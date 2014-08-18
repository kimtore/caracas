import zmq

context = zmq.Context()

#  Socket to talk to server
socket = context.socket(zmq.REQ)
socket.connect("tcp://raspberrypi:5551")

socket.send("")
message = int(socket.recv())

print "Ignition power: %s" % ("ON" if message & 1 else "OFF")
