#!/bin/bash

SERIAL=/dev/pts/4

echo -e "AT+CMGS=\"+48791942336\"\r\n" > ${SERIAL}
sleep 1
echo -e ">" > $SERIAL
sleep 1
echo -e "hello world 1\r\n" > $SERIAL
sleep 1
echo -e "+CMGS\r\n" > $SERIAL
sleep 1
echo -e "OK\r\n" > $SERIAL
sleep 1
