import signal
import time
import os

from NotificationManager import NotificationManager
from Alarm import Alarm
from Logger import Logger
from Logger import LogLevel

dirNameBrama = '/sharedfolders/MONITORING/brama_cam'
dirNameAltanka = '/sharedfolders/MONITORING/altanka_cam'

SMSDir = "/etc/scripts/SMS"

class Killer:
    kill_now = False
    def __init__(self):
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)
    
    def exit_gracefully(self, *args):
        self.kill_now = True

def splitSMS(fileAA):
    file = open(fileAA,"r")
    longSMS = file.read()
    file.close()
    SMSList = []
    smsTemp = ""
    for x in range(len(longSMS)):
        if x%150==0 and x>0:
            SMSList.append(smsTemp)
            smsTemp = ""
        else:
            smsTemp += longSMS[x]
    SMSList.append(smsTemp)

    i=1
    for message in SMSList:
        fileName="%s_%i"%(fileAA,i)
        newFile = open(fileName,"w")
        newFile.write(message)
        newFile.close()
        i+=1

    os.remove(fileAA)

def main():
    killer = Killer()

    Logger.settings(fileNameWihPath="/var/log/MonitoringManager.log", saveToFile=True, showFilename=True, logLevel=LogLevel.INFO, print=False)
    
    readyToNotifyBrama = True
    readyToNotifyAltanka = True

    notificationManager = NotificationManager("/etc/scripts/active_users.txt")
    alarm = Alarm()
    counter10s=0
    counter3min=0
    countFilesBrama=0
    countFilesAltanka=0

    alarmLevelBrama=0
    alarmLevelAltanka=0

    theNewestDirAltanka = alarm.getTheNewestDayDir(dirNameAltanka)
    if(theNewestDirAltanka == 0): 
        Logger.ERROR("Error with Disk")
        notificationManager.sendSMSAdmin("Error with Disk")
        exit()

    theNewestDirBrama = alarm.getTheNewestDayDir(dirNameBrama)
    countFilesBrama = alarm.getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)
    countFilesAltanka = alarm.getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)
    Logger.INFO("Ready")
    while not killer.kill_now:
        notificationManager.readAT()
        if (counter10s >= 20): # 10s
            counter10s=0
            counter3min+=1
            if(counter3min >= 18): # 18 x 10s = 3min
                counter3min = 0
                readyToNotifyAltanka = True
                readyToNotifyBrama = True

            newTheNewestDirAltanka = alarm.getTheNewestDayDir(dirNameAltanka)
            if(newTheNewestDirAltanka == 0):
                Logger.ERROR("Error with Disk")
                notificationManager.sendSMSAdmin("Error with Disk")
                break

            if (newTheNewestDirAltanka!= theNewestDirAltanka):  #new directory -> new day
                countFilesAltanka = 0
                theNewestDirAltanka=newTheNewestDirAltanka
            newCountFilesAltanka = alarm.getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)


            if(newCountFilesAltanka-countFilesAltanka>2 or alarmLevelAltanka>0):
                info = "Nope"
                if readyToNotifyAltanka:
                    if alarmLevelAltanka <= 1:
                        info="ALARM ALTANKA"
                    elif alarmLevelAltanka <= 4:
                        info="ALARM ALTANKA - ponownie"
                    elif alarmLevelAltanka <= 8:
                        info="ALARM ALATANKA - ktos nadal sie wluczy po podworku"
                    elif alarmLevelAltanka > 8:
                        info="ALARM ALTANKA - robisz impreze, czy co ? bardzo duzy ruch"
                    notificationManager.sendSMSNotification(info)
                    readyToNotifyAltanka = False
                    alarmLevelAltanka=0
                else:
                    info = "ALARM ALTANKA - log level +1"
                    alarmLevelAltanka+=1

                Logger.INFO(info)
                alarm.alarmLog(info)
            countFilesAltanka=newCountFilesAltanka

            newTheNewestDirBrama = alarm.getTheNewestDayDir(dirNameBrama)
            if (newTheNewestDirBrama != theNewestDirBrama):
                countFilesBrama = 0
                theNewestDirBrama=newTheNewestDirBrama
            newCountFilesBrama = alarm.getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)

            if(newCountFilesBrama-countFilesBrama>2 or alarmLevelBrama>0):
                info = "Nope"
                if readyToNotifyBrama:
                    if alarmLevelBrama <= 1:
                        info="ALARM BRAMA"
                    elif alarmLevelBrama <= 4:
                        info="ALARM BRAMA - ponownie"
                    elif alarmLevelBrama <= 8:
                        info="ALARM BRAMA - ktos nadal sie wluczy po podworku"
                    elif alarmLevelBrama > 8:
                        info="ALARM BRAMA - robisz impreze, czy co ? bardzo duzy ruch"
                    notificationManager.sendSMSNotification(info)
                    readyToNotifyBrama = False
                    alarmLevelBrama=0
                else:
                    info = "ALARM BRAMA - log Level +1"
                    alarmLevelBrama+=1

                Logger.INFO(info)
                alarm.alarmLog(info)
            countFilesBrama=newCountFilesBrama
        
        counter10s=counter10s+1
       
        if notificationManager.readyToSMS:
            listSMSFiles = os.listdir(SMSDir)
            for x in listSMSFiles:
                smsFile = os.path.join(SMSDir, x)
                file = open(smsFile,"r")
                text = file.read()
                file.close()
                if len(text) > 150:
                    splitSMS(smsFile)
                    continue
                notificationManager.sendSMSAdmin(text)
                os.remove(smsFile)
                notificationManager.readyToSMS=False
                break # next sms on other cycle, when GSM will ready to SMS    
     
        time.sleep(0.5)

#    notificationManager.phoneContacts.SaveToFile()
    notificationManager.saveToFile()
    Logger.INFO("exit program")

if __name__ == '__main__':
    main()
