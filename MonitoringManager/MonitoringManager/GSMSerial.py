import serial
import time
import os
import urllib.request
import sh
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

    def checkStatus(self):
        info = ""
        if not os.path.isdir(dirNameBrama):
            info += "disk is not mounted"
        if not self.checkNetwork():
            if len(info) > 3:
                info += "\n"
            info += "I'm not connected to network \n" + self.checkMemory()
    
        if len(info) < 3:
            return "everything okay \n" + self.checkMemory()
        else:
            return info

class GSMSerial:
    #serialGSM = serial.Serial('/dev/virtualcom0',19200)  # open serial port   
    def __init__(self):
        self.serialGSM = serial.Serial('/dev/ttyAMA0',19200)  # open serial port   
        commend1='AT\r\n'
        self.serialGSM.write(commend1.encode())
        while self.serialGSM.inWaiting() <= 1:
            time.sleep(0.01)
        self.serialGSM.read(self.serialGSM.inWaiting())
        time.sleep(0.5)

        commend1='AT+CMGF=1\r\n'
        self.serialGSM.write(commend1.encode())
        while self.serialGSM.inWaiting() <= 1:
            time.sleep(0.01)
        self.serialGSM.read(self.serialGSM.inWaiting())
        time.sleep(0.5)

        commend2='AT+CNMI=1,2,0,0\r\n'
        self.serialGSM.write(commend2.encode())
        while self.serialGSM.inWaiting()<=1:
            time.sleep(0.01)
        self.serialGSM.read(self.serialGSM.inWaiting())
        time.sleep(0.5)

        commend3='AT+CLIP=1\r\n'
        self.serialGSM.write(commend3.encode())
        while self.serialGSM.inWaiting()<=1:
            time.sleep(0.01)
        time.sleep(0.5)

    def sendSMS(self, number, text):
        commend='AT+CMGS="' + str(number) + '"\r\n'
        Logger.INFO("sending SMS to " + str(number))
        self.serialGSM.write(commend.encode())
        while self.serialGSM.inWaiting()<=1:
            time.sleep(0.04)
        time.sleep(0.2)
        data_str = self.serialGSM.read(self.serialGSM.inWaiting())
        data_str = data_str.decode("utf-8")
        if ('>' in data_str):
            self.serialGSM.write(text.encode()) #\x1a
            self.serialGSM.write(bytes(b'\x1a'))

    def readMessages(self):
        if (self.serialGSM.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
            # NB: for PySerial v3.0 or later, use property `in_waiting` instead of function `inWaiting()` below!
            #data_str = ser.read(ser.inWaiting()).decode('ascii') #read the bytes and convert from binary array to ASCII
            data_str = self.serialGSM.read(self.serialGSM.inWaiting())
            data_str = data_str.decode("utf-8")
            Logger.INFO(data_str)
            return data_str
        return None    

if __name__ == '__main__':
    print("GSMSerial")

