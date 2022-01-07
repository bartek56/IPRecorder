import serial
import time
import os
import datetime
import urllib
import sh

dirNameBrama = '/sharedfolders/MONITORING/brama_cam'
dirNameAltanka = '/sharedfolders/MONITORING/altanka_cam'

ADMIN_NUMBER=999999999
USER_NUMBER=999999999


class IpRecorderStatus:
    def __init__(self):
        pass

    def checkNetwork(self):
        try:
            urllib.urlopen("http://google.com") #Python 3.x
            return True
        except:
            return False

    def checkMemory(self):
        rootMemory = sh.awk( sh.grep(sh.df("-h"), "root"), "{print $3 \"/\" $2}")
        diskMemory = sh.awk( sh.grep(sh.df("-h"), "INTENSO"), "{print $3 \"/\" $2}")
        rootMemory = str(rootMemory).strip()
        diskMemory = str(diskMemory).strip()
        return ("External Memory: " + diskMemory + "\n" + "Internal Memory: " + rootMemory)

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
    ipRecorderStatus = IpRecorderStatus()
    serialGSM = serial.Serial('/dev/ttyAMA0',19200)  # open serial port   
    #serialGSM = serial.Serial('/dev/virtualcom0',19200)  # open serial port   
    def __init__(self):
        print("init")
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
        self.GSMLog("sending SMS \""+ text + "\" to " + str(number))
        self.serialGSM.write(commend.encode())
        while self.serialGSM.inWaiting()<=1:
            time.sleep(0.04)
        time.sleep(0.2)
        data_str = self.serialGSM.read(self.serialGSM.inWaiting())
        if ('>' in data_str):
            self.serialGSM.write(text.encode()) #\x1a
            self.serialGSM.write(b'\x1a')

    def sendSMSAdmin(self, text):
        self.sendSMS(ADMIN_NUMBER, text)

    def sendSMSUser(self, text):
        self.sendSMS(USER_NUMBER, text)
    
    def readMessages(self):
        if (self.serialGSM.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
            # NB: for PySerial v3.0 or later, use property `in_waiting` instead of function `inWaiting()` below!
            #data_str = ser.read(ser.inWaiting()).decode('ascii') #read the bytes and convert from binary array to ASCII
            data_str = self.serialGSM.read(self.serialGSM.inWaiting())
            self.GSMLog(data_str)
            if('ALARMOFF' in data_str):
                ALARMON=False
                self.sendSMSUser("Wylaczono ALARM")
            elif('ALARMON' in data_str):
                ALARMON=True
                self.sendSMSUser("Wlaczono ALARM")
            elif('STATUS' in data_str):
                status = self.ipRecorderStatus.checkStatus()
                self.sendSMSAdmin(status)
            elif('+CLIP:' in data_str):
                self.sendSMSAdmin("somebody call to me: \n" + data_str)
                #elif('AT+' not in data_str):
                #sendSMSAdmin("I don't understand \n" + data_str)

    def GSMLog(self, info):
        date_time = datetime.datetime.now().strftime("%Y/%m/%d, %H:%M:%S")
        answer = date_time + ": " + info + '\n'
        file = open("/var/log/GSMSerial.log",'a')
        #print(answer)
        file.writelines(answer)
        file.close()
 

class Alarm:
    def __init__(self):
        pass

    def getTheNewestDayDir(self, dirName):
        if os.path.isdir(dirName):
            dirs = [d for d in os.listdir(dirName)]
            dirs.remove('DVRWorkDirectory')
            latest_dir=max(dirs, key=os.path.basename)
            return latest_dir
        else:
            return 0

    def getListOfFiles(self, dirName):
        if os.path.isdir(dirName):
            listOfFiles = list()
            for (dirpath, dirnames, filenames) in os.walk(dirName):
                listOfFiles += [os.path.join(dirpath, f) for f in os.listdir(dirpath) if f.endswith('.jpg')]
            return len(listOfFiles)
        else:
            return 0

    def alarmLog(self, info):
        date_time = datetime.datetime.now().strftime("%Y/%m/%d, %H:%M:%S")
        answer = date_time + ": " + info + '\n'
        file = open("/var/log/Alarm.log",'a')
        #print(answer)
        file.writelines(answer)
        file.close()


def main():
    ALARMON = False
    gsm = GSMSerial()
    time.sleep(3)
    alarm = Alarm()
    counter=0
    counterAltanka=0
    counterBrama=0
    countFilesBrama=0
    countFilesAltanka=0
    theNewestDirAltanka = alarm.getTheNewestDayDir(dirNameAltanka)
    if(theNewestDirAltanka == 0): 
        gsm.sendSMSAdmin("Error with Disk")
        exit()

    theNewestDirBrama = alarm.getTheNewestDayDir(dirNameBrama)
    countFilesBrama = alarm.getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)
    countFilesAltanka = alarm.getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)
    print("Ready")
    while (True):
        gsm.readMessages()
        if (counter > 20):
            counter=0
            newTheNewestDirAltanka = alarm.getTheNewestDayDir(dirNameAltanka)
            if(newTheNewestDirAltanka == 0):
                gsm.sendSMSAdmin("Error with Disk")
                exit()

            if (newTheNewestDirAltanka!= theNewestDirAltanka):  #new directory -> new day
                countFilesAltanka = 0
                theNewestDirAltanka=newTheNewestDirAltanka
            newCountFilesAltanka = alarm.getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)
            if(newCountFilesAltanka == 0):
                gsm.sendSMSAdmin("Error with Disk")
                exit()

            if(newCountFilesAltanka-countFilesAltanka==2): # come in to loop, when somebody come in to possesion
                time.sleep(1.5)

            if(newCountFilesAltanka-countFilesAltanka>2):
                info = "ALARM ALTANKA"
                if(ALARMON==True):
                    gsm.sendSMSUser(info)
                alarm.alarmLog(info)
            countFilesAltanka=newCountFilesAltanka

            newTheNewestDirBrama = alarm.getTheNewestDayDir(dirNameBrama)
            if (newTheNewestDirBrama != theNewestDirBrama):
                countFilesBrama = 0
                theNewestDirBrama=newTheNewestDirBrama
            newCountFilesBrama = alarm.getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)

            if(newCountFilesBrama-countFilesBrama>2):
                info = "ALARM BRAMA"
                if(ALARMON==True):
                    gsm.sendSMSUser(info)
                alarm.alarmLog(info)
            countFilesBrama=newCountFilesBrama
        
        counter=counter+1
       
        if(os.path.exists("/etc/scripts/SMS.txt")):
            file = open("/etc/scripts/SMS.txt","r")
            text = file.read()
            file.close()
            gsm.sendSMSAdmin(text)
            os.remove("/etc/scripts/SMS.txt")
        
        time.sleep(0.5)

if __name__ == '__main__':
    main()

