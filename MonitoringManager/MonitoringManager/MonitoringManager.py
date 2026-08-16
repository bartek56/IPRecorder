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


def processCamera(camera, readyToNotify, notificationManager):
    wrapper = [readyToNotify]
    result = camera.analyzeMoving(wrapper)
    readyToNotify = wrapper[0]

    if result:
        Logger.DEBUG(result)
        notificationManager.sendSMSNotification(result)

    return readyToNotify


def updateNotificationBlockState(cameraName, readyToNotify, notificationBlockStart, now, blockDuration=60.0):
    if not readyToNotify:
        if notificationBlockStart is None:
            notificationBlockStart = now
        elif now - notificationBlockStart >= blockDuration:
            Logger.DEBUG(f"{cameraName}: 1 min")
            notificationBlockStart = None
            readyToNotify = True
    else:
        notificationBlockStart = None

    return readyToNotify, notificationBlockStart


def main():
    killer = Killer()

    Logger.settings(fileNameWihPath=CONFIG.LOGFile, saveToFile=False, showFilename=True, logLevel=LogLevel.INFO, print=True)
    Logger.INFO(" ---------------- Start Monitoring ------------------ ")

    notificationManager = NotificationManager(CONFIG.ACTIVE_USERS_FILE, CONFIG.CONTACTS_FILE, CONFIG.GSMSerial, CONFIG.ADMIN_NUMBER)

    readyToNotifyBrama = True
    readyToNotifyAltanka = True
    cameraAltanka = CameraAnalyzer(CONFIG.dirNameAltanka, "ALTANKA", CONFIG.ALARM_LOG_FILE)
    cameraBrama = CameraAnalyzer(CONFIG.dirNameBrama, "BRAMA", CONFIG.ALARM_LOG_FILE)
    cameraCheckInterval = 5.0
    notificationBlockStartAltanka = None
    notificationBlockStartBrama = None
    nextCameraCheckAt = time.monotonic() + cameraCheckInterval

    if(cameraAltanka.theNewestDir == 0):
        Logger.ERROR("Error with Disk")
        notificationManager.sendSMSAdmin("Error with Disk")
        return

    Logger.INFO("-------------- Initialization was finished -----------------")
    while not killer.kill_now:
        notificationManager.checkNewMessage()
        notificationManager.checkNewCall()

        now = time.monotonic()
        if now >= nextCameraCheckAt:
            Logger.DEBUG(f"{cameraCheckInterval} sec")
            nextCameraCheckAt = now + cameraCheckInterval


            readyToNotifyAltanka, notificationBlockStartAltanka = updateNotificationBlockState(
                "ALTANKA",
                readyToNotifyAltanka,
                notificationBlockStartAltanka,
                now,
            )
            readyToNotifyAltanka = processCamera(cameraAltanka, readyToNotifyAltanka, notificationManager)
            

            readyToNotifyBrama, notificationBlockStartBrama = updateNotificationBlockState(
                "BRAMA",
                readyToNotifyBrama,
                notificationBlockStartBrama,
                now,
            )
            readyToNotifyBrama = processCamera(cameraBrama, readyToNotifyBrama, notificationManager)


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

        time.sleep(0.1)

    notificationManager.saveToFile()
    Logger.INFO("-------------------- exit program ---------------------")

if __name__ == '__main__':
    main()
