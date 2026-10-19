#!/bin/sh
# Runs ON a site frontend: read a node's serial port until "#END" (max 2 days).
# Every line is prefixed "receive_time;node;". Port 20000 takes one client.
#   sh capture.sh <node-host e.g. samr21-3> <logfile>
host=$1 log=$2
timeout 172800 nc "$host" 20000 | awk -v h="$host" '{ sub(/\r$/, ""); printf "%.3f;%s;%s\n", systime(), h, $0; fflush() } /#END$/ { exit }' > "$log"
