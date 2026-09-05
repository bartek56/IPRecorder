[![CircleCI](https://img.shields.io/circleci/build/github/bartek56/IPRecorder)](https://app.circleci.com/pipelines/github/bartek56/IPRecorder)

[![codecov](https://codecov.io/gh/bartek56/IPRecorder/branch/main/graph/badge.svg)](https://codecov.io/gh/bartek56/IPRecorder)

# IPRecorder

IPRecorder verifies newly created images from an IP camera (motion-triggered)
using an AI-based object detector (YOLOv8n, model file `yolov8n.pt`) to detect
objects such as people, vehicles and other motion-relevant objects, and
notifies by SMS when a new relevant image is detected. You can test the
system using an AT command simulator such as the AT-Emulator on Ubuntu.

## Requirements

- Python 3.10+ (or compatible)
- pybind11
- spdlog
- CMake
- A working SMS/GSM modem or an AT simulator for testing

## Quick Start

1. Clone the repository and fetch submodules:

```bash
git clone --recurse-submodules https://github.com/bartek56/IPRecorder.git
cd IPRecorder
git submodule update --init --recursive
```

2. Build and install `GSMEngine` (C++/Python bindings):

```bash
cd GSMEngine
sudo pip3 install . --break-system-packages
cmake -S . -B build
cmake --build build
```

3. Configure devices and settings

Edit the configuration file at `MonitoringManager/MonitoringManager/Config.py` to
set camera, serial port, and SMS parameters.

4. Build and install `MonitoringManager`:

```bash
cd ../MonitoringManager
sudo pip3 install . --break-system-packages
```

5. Start the system service

```bash
sudo systemctl start monitoring.service
sudo systemctl enable monitoring.service
sudo systemctl status monitoring.service
```

## Testing (virtual serial)

To create a pair of virtual serial ports for testing, use `socat`:

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

This prints two device paths (e.g. `/dev/pts/X` and `/dev/pts/Y`) — use one
end for the emulator and the other for the application.

## Notes

- If you are using the AT-Emulator for testing, configure the emulator to
	match the serial settings used by `MonitoringManager`.
- Use virtual environments or containerization to avoid system-wide package
	conflicts when running `pip3 install .`.

## Contributing

Feel free to open issues or pull requests. For development, run tests in the
`MonitoringManager/tests/` directory.


