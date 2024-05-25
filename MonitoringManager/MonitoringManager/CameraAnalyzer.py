import os
import datetime
from Logger import Logger

class CameraAnalyzer():
    def __init__(self, dirName, cameraName, logFile):
        self.dirName = dirName
        self.cameraName = cameraName
        self.theNewestDir = self.getTheNewestDayDir(self.dirName)
        self.logFile = logFile
        if(self.theNewestDir == 0):
            Logger.ERROR("Error with Disk")
        else:
            self.countFiles = self.getListOfFiles(self.dirName+'/'+self.theNewestDir)
            self.alarmLevel = 0
            self.alarmLevelActive = False

    def analyzeMoving(self, readyToNotify):
            smsMessage = None
            newTheNewestDir = self.getTheNewestDayDir(self.dirName)

            if(newTheNewestDir == 0):
                Logger.ERROR("Error with Disk")
                return "ERROR with Disk"

            if (newTheNewestDir!= self.theNewestDir):  #new directory -> new day
                self.countFiles = 0
                self.theNewestDir=newTheNewestDir
            newCountFiles = self.getListOfFiles(self.dirName+'/'+self.theNewestDir)
            Logger.DEBUG("old count of files:", self.countFiles)
            Logger.DEBUG("new count of files:", newCountFiles)

            if(newCountFiles-self.countFiles>=3):
                self.alarmLevelActive = True
                if not readyToNotify[0]:
                    info = "ALARM " + self.cameraName + "- log level +1"
                    self.alarmLevel+=1
                    Logger.INFO(info)
                    self.alarmLog(info)

            if readyToNotify[0] and self.alarmLevelActive:
                self.alarmLevelActive = False
                if self.alarmLevel == 0:
                    info="ALARM " + self.cameraName
                elif self.alarmLevel <= 1:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - bardzo maly ruch, mogl to byc kot"
                elif self.alarmLevel <= 4:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - ktos nadal sie wluczy po podworku, sprawdz zdjecia"
                elif self.alarmLevel > 4:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - robisz impreze, czy co ? bardzo duzy ruch"
                smsMessage = info
                readyToNotify[0] = False

                self.alarmLevel=0

                Logger.INFO(info)
                self.alarmLog(info)
            self.countFiles=newCountFiles
            return smsMessage

    def getTheNewestDayDir(self, dirName):
        if os.path.isdir(dirName):
            dirs = [d for d in os.listdir(dirName)]
            if "DVRWorkDirectory" in dirs:
                dirs.remove('DVRWorkDirectory')
            if(len(dirs)==0):
                Logger.WARNING("Directory", dirName, "is empty")
                return 0
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
        file = open(self.logFile,'a')
        file.writelines(answer)
        file.close()
