import os
import nmap

class NetworkMonitoring():
    def __init__(self):
        self.knownDevicesFileName = "/etc/scripts/known_devices"

    def scanDevicesInLocalNetwork(self):
        nm = nmap.PortScanner()
        nm.scan(hosts='192.168.1.0/24',arguments='-sn')
        devices = {}
        for x in nm.all_hosts():
            addresses = nm[x]['addresses']
            hostname = nm[x]['hostnames'][0]['name']
            if "IpRecorder" not in hostname:
                devices[addresses['mac']] = addresses['ipv4']
        return devices

    def analyzeNetwork(self):
        if not os.path.isfile(self.knownDevicesFileName):
            fileDevices = open(self.knownDevicesFileName, "w")
            fileDevices.write("")
            fileDevices.close()

        devicesFile = open(self.knownDevicesFileName,'r')
        devices = self.scanDevicesInLocalNetwork()
        listDevices = devicesFile.readlines()
        listKnownDevices = []
        for x in listDevices:
            deviceInfo = x.split(" ")
            listKnownDevices.append(deviceInfo[0])
        unknownDevices = {}
        for actualDevice in devices.keys():
            if actualDevice not in listKnownDevices:
                unknownDevices[actualDevice] = devices[actualDevice]
        return unknownDevices

if __name__ == "__main__":
    net = NetworkMonitoring()
    unknownDevices = net.analyzeNetwork()
    devices = ""
    for mac, ip in unknownDevices.items():
        devices += ip
        devices += " "
        devices += mac
        devices += "\n"

    if len(devices) > 1:
        fileSMS = open("/etc/scripts/SMS/SMSNetwork","w")
        fileSMS.write("new device in Network:")
        fileSMS.write(devices)
        fileSMS.close()

