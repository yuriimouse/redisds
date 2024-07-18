#!/bin/bash

pkill rsyslogd
sleep 3
yes | cp etc/rsyslog.conf /etc/
rsyslogd
