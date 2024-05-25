#!/bin/bash

SERIAL=/dev/pts/3
MESSAGE="hello world"

mode=$1

if [[ $mode -eq 1 ]] ; then
    echo 'init'
    echo -e "AT\r\n" > ${SERIAL}
    echo -e "OK\r\n" > ${SERIAL}
    exit 1
fi

if [[ $mode -eq 2 ]] ; then
    echo 'receive message'
    echo -e "AT+CMGS=\"+48791942336\"\r\n" > ${SERIAL}
    sleep 1
    echo -e ">" > $SERIAL
    sleep 1
    echo -e "$MESSAGE\r\n" > $SERIAL
    sleep 1
    echo -e "+CMGS\r\n" > $SERIAL
    sleep 1
    echo -e "OK\r\n" > $SERIAL
    exit 1
fi

if [[ $mode -eq 3 ]] ; then
    echo "send message and wait for response"
    echo -e "+CMT: \"+48791942336\",,\"23/05/2025,12:00:00+00\"\r\n" > ${SERIAL}
    sleep 1
    echo -e "$MESSAGE\r\n" > $SERIAL
    sleep 2

    echo -e "AT+CMGS=\"+48791942336\"\r\n" > ${SERIAL}
    sleep 1
    echo -e ">" > $SERIAL
    sleep 1
    echo -e "thanks for message\r\n" > $SERIAL
    sleep 1
    echo -e "+CMGS\r\n" > $SERIAL
    sleep 1
    echo -e "OK\r\n" > $SERIAL
    exit 1


    exit 1
fi
