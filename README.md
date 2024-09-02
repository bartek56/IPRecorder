# IpRecorder

IPRecorder is a software to verify new image (created by IP Camera during moving) and inform by SMS about it
You can test it with [AT simulator](https://github.com/celersms/AT-Emulator) on Ubuntu

Requirements:
- pybind11
- spdlog

1. Download submodules

git submodule update --init

2. Build and install GSMEngine

cd GSMEngine
sudo pip3 install . --break-system-packages
cmake -S . -B build
cmake --build build

3. Configure devices

Edit parameters in MonitoringManager/MonitoringManager/Config.py file.

4. Build and install MonitoringManager

cd MonitoringManager
sudo pip3 install . --break-system-packages

5. Start service

systemctl start monitoring.service


For Testing:
virtual serial:
socat -d -d pty,raw,echo=0 pty,raw,echo=0

