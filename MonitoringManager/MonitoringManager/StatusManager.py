import os
import urllib.request
import sh
import subprocess

from Config import dirNameBrama

class IpRecorderStatus:
    def __init__(self):
        pass

    def checkNetwork(self):
        try:
            urllib.request.urlopen("http://google.com") #Python 3.x
            return True
        except:
           return False

    def checkMemory(self):
        # df -h | grep /dev/mmcblk0p2 | awk '{print $3 "/" $2}'
        disks = sh.df('-h')
        grep = sh.grep('sda3', _in=disks)
        grep = str(grep).strip()
        rootMemory = sh.awk("{print $3 \"/\" $2}", _in=grep)
        rootMemory = str(rootMemory).strip()

        grep = sh.grep("intenso",_in=disks)
        grep = str(grep).strip()
        diskMemory = sh.awk("{print $3 \"/\" $2}", _in=grep)
        diskMemory = str(diskMemory).strip()

        return ("Internal Memory: " + rootMemory + "\n" +
                "External Memory: " + diskMemory)

    def checkFtpActiveClients(self):
        process = subprocess.run("netstat -tanp | grep vsftpd | awk '{print $5}' | cut -d: -f1 | sort -u",
                                 shell=True, stdout=subprocess.PIPE, universal_newlines=True)
        result = process.stdout
        resultStr = ""
        if "192.168.1.6" not in result:
            resultStr += "camera1 not available"
        if "192.168.1.7" not in result:
            if len(resultStr) > 2:
                resultStr += "\n"
            resultStr += "camera2 not available"
        return resultStr

    def checkStatus(self):
        actualStatus = True
        info = ""
        diskIsMounted = os.path.isdir(dirNameBrama)
        if not diskIsMounted:
            info += "disk is not mounted"
            actualStatus = False

        if not self.checkNetwork():
            if len(info) > 3:
                info += "\n"
            info += "I'm not connected to network"
            actualStatus = False

        result = self.checkFtpActiveClients()
        if(len(result) > 3):
            actualStatus = False
            if len(info) > 3:
                info += "\n"
            info += result

        if diskIsMounted:
            if len(info) > 3:
                info += "\n"
            info += self.checkMemory()

        if actualStatus:
            return "everything okay \n" + info
        else:
            return info

if __name__ == '__main__':
    print("GSMSerial")
    status = IpRecorderStatus()
    print(status.checkStatus())
