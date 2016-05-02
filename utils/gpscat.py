#!/usr/bin/env python2.7
#

import time
import socket
import argparse

def main():
    description = """
    Stream a file with recorded NMEA sentences on a TCP server suitable for
    gpsd, sleeping N seconds between each update.
    """
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--sleep', type=float, default=1, required=False, help='How many seconds between each update')
    parser.add_argument('file', action='store', help='File with NMEA sentences')
    parser.add_argument('bind', action='store', help='TCP socket to listen on, e.g. localhost:3333')
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
                    time.sleep(args.sleep)
                connection.sendall(line)
                print line,
        connection.close()
        print 'Connection closed'

if __name__ == '__main__':
    main()
