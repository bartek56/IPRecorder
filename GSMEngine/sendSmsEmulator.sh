#!/bin/bash

SERIAL=/dev/pts/4

echo -e "+CMT: \"+48791942336\",,\"23/05/2025,12:00:00+00\"\r\n" > ${SERIAL}
sleep 1
echo -e "test SMS\r\n" > $SERIAL
