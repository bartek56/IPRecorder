#!/bin/bash


#Sys logs --> once a week
truncate -s 0 /var/log/debug
truncate -s 0 /var/log/syslog
#truncate -s 0 /var/log/messages
truncate -s 0 /var/log/daemon.log
truncate -s 0 /var/log/auth.log
#truncate -s 0 /var/log/kern.log
truncate -s 0 /var/log/user.log
truncate -s 0 /var/log/proftpd/vroot.log
truncate -s 0 /var/log/proftpd.log
