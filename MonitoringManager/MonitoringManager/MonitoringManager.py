import signal
import time
import os

from NotificationManager import NotificationManager
from CameraAnalyzer import CameraAnalyzer
from Logger import Logger
from Logger import LogLevel
import Config as CONFIG

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

    Logger.settings(fileNameWihPath=CONFIG.LOGFile, saveToFile=False, showFilename=True, logLevel=LogLevel.INFO, print=True)
    Logger.INFO(" ---------------- Start Monitoring ------------------ ")

    notificationManager = NotificationManager(CONFIG.ACTIVE_USERS_FILE, CONFIG.CONTACTS_FILE, CONFIG.GSMSerial, CONFIG.ADMIN_NUMBER)

    counter4s=0
    counter1min=0

    readyToNotifyBrama = True
    readyToNotifyAltanka = True
    cameraAltanka = CameraAnalyzer(CONFIG.dirNameAltanka, "ALTANKA", CONFIG.ALARM_LOG_FILE)
    cameraBrama = CameraAnalyzer(CONFIG.dirNameBrama, "BRAMA", CONFIG.ALARM_LOG_FILE)

    if(cameraAltanka.theNewestDir == 0):
        Logger.ERROR("Error with Disk")
        notificationManager.sendSMSAdmin("Error with Disk")
        exit()

    Logger.INFO("-------------- Initialization was finished -----------------")
    while not killer.kill_now:
        notificationManager.checkNewMessage()
        if (counter4s >= 8): # 8 x 0.5s = 4s
            Logger.DEBUG("4 sec")
            counter4s=0
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


        counter4s=counter4s+1

        if notificationManager.readyToSMS:
            listSMSFiles = os.listdir(CONFIG.SMSDir)
            for x in listSMSFiles:
                smsFile = os.path.join(CONFIG.SMSDir, x)
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

    notificationManager.saveToFile()
    Logger.INFO("-------------------- exit program ---------------------")

if __name__ == '__main__':
    main()
