import serial
import time
import os
import urllib.request
import sh
import subprocess
from Logger import Logger

dirNameBrama = '/sharedfolders/MONITORING/brama_cam'
dirNameAltanka = '/sharedfolders/MONITORING/altanka_cam'


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
        rootMemory = sh.awk( sh.grep(sh.df("-h"), "root"), "{print $3 \"/\" $2}")
        diskMemory = sh.awk( sh.grep(sh.df("-h"), "INTENSO"), "{print $3 \"/\" $2}")
        ramMemory = sh.awk( sh.grep(sh.df("-h"), "var/log"), "{print $3 \"/\" $2}") 
        rootMemory = str(rootMemory).strip()
        diskMemory = str(diskMemory).strip()
        ramMemory = str(ramMemory).strip()
        return ("External Memory: " + diskMemory + "\n" + 
                "Internal Memory: " + rootMemory + "\n" +
                "Logs memory: " + ramMemory)

    def checkFtpActiveClients(self):
        #ftpwho -v -o oneline
        process = subprocess.run('ftpwho -v -o oneline', shell=True, stdout=subprocess.PIPE, universal_newlines=True)
        result = process.stdout
        resultStr = ""
        if "[192.168.1.6]" not in result:
            resultStr += "camera1 not available" 
        if "[192.168.1.7]" not in result:
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
