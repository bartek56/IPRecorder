# IpRecorder
scripts for IpRecorder (OpenMediaVault)

Requirements:

- boost.python
1. Download bootstrap

2. Configure
./bootstrap.sh --with-python=python3 --prefix=<install_prefix>

3. Build and install Bootstrap
./b2 --with-python -j1 install

4. build C++ library
cd GSMEngine
g++ -shared -o rectangle.so -fPIC rectangleWrapper.cpp -I<install_prefix>/include -I<python_dev_dir> -L<install_prefix>/lib -lboost_python3

5. Run
LD_LIBRARY_PATH=<install_prefix>/lib python script.py

GSMSerial.py:
It's script to verify new image (created by IP Camera during moving) and inform by SMS about it
You can test it with [AT simulator](https://github.com/celersms/AT-Emulator) on Ubuntu 

GSMSerial.service
systemd service to auto start GSMSerial.py

autoRemove.sh
script to remove videos/images after 10 days

clearLogs.sh, clearSysLogs.sh
clear contains in log files



