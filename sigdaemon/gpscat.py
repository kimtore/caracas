#!/usr/bin/env python2.7
#
# Output a file with NMEA sentences, sleeping one second between each chunk
#

import time
import socket
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', action='store')
    parser.add_argument('bind', action='store')
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host, port = args.bind.split(':')
    sock.bind((host, int(port)))
    sock.listen(1)

    while True:
        connection, address = sock.accept()
        print 'Received connection from %s:%d' % address
        with open(args.file, 'r') as f:
            while True:
                line = f.readline()
                if len(line) == 0:
                    break
                if line[:6] == '$GPGGA':
                    time.sleep(1)
                connection.sendall(line)
                print line,
        connection.close()
        print 'Connection closed'

if __name__ == '__main__':
    main()
