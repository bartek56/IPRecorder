import signal
import time
import os

from NotificationManager import NotificationManager
from CameraAnalyzer import CameraAnalyzer
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
    notificationManager = NotificationManager("/etc/scripts/active_users.txt")

    counter6s=0
    counter1min=0

    readyToNotifyBrama = True
    readyToNotifyAltanka = True
    cameraAltanka = CameraAnalyzer(dirNameAltanka, "ALTANKA")
    cameraBrama = CameraAnalyzer(dirNameBrama, "BRAMA")

    if(cameraAltanka.theNewestDir == 0):
        Logger.ERROR("Error with Disk")
        notificationManager.sendSMSAdmin("Error with Disk")
        exit()

    Logger.INFO("Ready")
    while not killer.kill_now:
        notificationManager.checkNewMessage()
        if (counter6s >= 12): # 12 x 0.5s = 6s
            Logger.DEBUG("6 sec")
            counter6s=0
            if not readyToNotifyAltanka or not readyToNotifyBrama:
                counter1min+=1
                if(counter1min >= 10): # 10 x 6s = 1min
                    Logger.DEBUG("1 min")
                    counter1min = 0
                    readyToNotifyAltanka = True
                    readyToNotifyBrama = True

            wrapper = [readyToNotifyAltanka]
            result = cameraAltanka.analyzeMoving(wrapper)
            readyToNotifyAltanka = wrapper[0]

            if result:
                Logger.DEBUG(result)
                notificationManager.sendSMSNotification(result)
                if "ERROR" in result:
                    notificationManager.sendSMSAdmin(result)
                    Logger.ERROR(result)
                    os.system("reboot now")
                    break

            wrapper = [readyToNotifyBrama]
            result = cameraBrama.analyzeMoving(wrapper)
            readyToNotifyBrama = wrapper[0]
            if result:
                Logger.DEBUG(result)
                notificationManager.sendSMSNotification(result)
                if "ERROR" in result:
                    notificationManager.sendSMSAdmin(result)
                    Logger.ERROR(result)
                    os.system('reboot now')
                    break


        counter6s=counter6s+1

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
