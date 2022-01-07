# IpRecorder
scripts for IpRecorder (OpenMediaVault)

GSMSerial.py:
It's script to verify new image (created by IP Camera during moving) and inform by SMS about it
You can test it with [AT simulator](https://github.com/celersms/AT-Emulator) on Ubuntu 

GSMSerial.service
systemd service to auto start GSMSerial.py

autoRemove.sh
script to remove videos/images after 10 days

clearLogs.sh, clearSysLogs.sh
clear contains in log files



