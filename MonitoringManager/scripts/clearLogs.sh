#!/bin/bash


#Sys logs --> once a week
truncate -s 0 /var/log/debug
truncate -s 0 /var/log/syslog
truncate -s 0 /var/log/messages
truncate -s 0 /var/log/daemon.log
truncate -s 0 /var/log/auth.log
truncate -s 0 /var/log/kern.log
truncate -s 0 /var/log/user.log
truncate -s 0 /var/log/proftpd/vroot.log
truncate -s 0 /var/log/proftpd.log


#logs --> once a month
truncate -s 0 /var/log/autoRemove.log
truncate -s 0 /var/log/Alarm.log
truncate -s 0 /var/log/minidlna.log
truncate -s 0 /var/log/php7.0-fpm.log
truncate -s 0 /var/log/dpkg.log
truncate -s 0 /var/log/MonitoringManager.log
truncate -s 0 /var/log/mail.err
truncate -s 0 /var/log/mail.info
truncate -s 0 /var/log/mail.log
truncate -s 0 /var/log/mail.warn
truncate -s 0 /var/log/cron-apt/log
truncate -s 0 /var/log/nginx/openmediavault-webgui_access.log
truncate -s 0 /var/log/nginx/openmediavault-webgui_error.log
