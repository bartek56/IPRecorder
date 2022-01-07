#!/bin/bash


#once a week
truncate -s 0 /var/log/debug
truncate -s 0 /var/log/syslog
#truncate -s 0 /var/log/messages
truncate -s 0 /var/log/daemon.log
truncate -s 0 /var/log/auth.log
#truncate -s 0 /var/log/kern.log
truncate -s 0 /var/log/user.log
truncate -s 0 /var/log/proftpd/vroot.log

#once a month

#truncate -s 0 /var/log/autoRemove.log
#truncate -s 0 /var/log/Alarm.log
#truncate -s 0 /var/log/minidlna.log
#truncate -s 0 /var/log/php7.0-fpm.log
#truncate -s 0 /var/log/dpkg.log
#truncate -s 0 /var/log/GSMSerial.log
#truncate -s 0 /var/log/proftpd.log


