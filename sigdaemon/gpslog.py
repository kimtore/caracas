#!/usr/bin/env python2.7
#
# Log coordinates to syslog every N seconds

import gps
import syslog
import datetime


LOG_INTERVAL = 60


def add_attr(report, line, key):
    if hasattr(report, key):
        line += [(key, report[key])]

def make_report_line(report):
    output = []
    [add_attr(report, output, key) for key in ['time', 'lat', 'lon', 'alt', 'speed']]
    line = ' '.join(['%s=%s' % (k, v) for k, v in output])
    line = "%dD fix: %s" % (report['mode'], line)
    return line

if __name__ == '__main__':
    syslog.openlog('gpslog', syslog.LOG_PID, syslog.LOG_DAEMON)
    session = gps.gps()
    session.stream(gps.WATCH_ENABLE | gps.WATCH_NEWSTYLE)
    report = {}
    then = datetime.datetime.fromtimestamp(0)

    try:
        for r in session:
            if r['class'] != 'TPV':
                continue
            report = r
            now = datetime.datetime.now()
            delta = now - then
            if delta.total_seconds() >= LOG_INTERVAL:
                then = now
                syslog.syslog(make_report_line(report))
    except Exception, e:
        syslog.syslog("Uncaught exception, terminating!")
        syslog.syslog(unicode(e))
