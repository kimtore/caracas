#!/bin/sh
/usr/local/bin/adb shell dumpsys location | grep -A1 "Last Known Locations" -m 1 | sed 1d | sed -e 's/^\s\+//' | logger -t location
