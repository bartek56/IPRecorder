import os
import time
import datetime
from dataclasses import dataclass
from Logger import Logger
import DetectObjects

@dataclass
class CameraAnalysisResult:
    cameraName: str
    movementLevel: int
    firstDetection: bool
    message: str
    reasons: str
    hasReasons: bool
    readyToNotify: bool = True
    error: str | None = None


class CameraAnalyzer():
    def __init__(self, dirName, cameraName, logFile, minNewFilesToDetect, notificationBlockDuration):
        """
        Initialize the camera analyzer.

        Args:
            dirName (str): Path to the directory containing camera recordings.
            cameraName (str): Human-readable camera identifier used in logs and alerts.
            logFile (str): Path to the file used for alarm logging.
            minNewFilesToDetect (int): Minimum number of newly created files required to trigger detection (count of JPG files).
            notificationBlockDuration (float): Time in seconds for which notifications are blocked after an alert (seconds).
        """
        self.dirName = dirName
        self.cameraName = cameraName
        self.logFile = logFile
        self.notificationBlockDuration = notificationBlockDuration
        self.minNewFilesToDetect = minNewFilesToDetect

        self.readyToNotify = True
        self.notificationBlockStart = None
        self.theNewestDir = self.getTheNewestDayDir(self.dirName)
        if(self.theNewestDir == 0):
            Logger.ERROR("Error with Disk")
        else:
            self.countFiles = self.getListOfFiles(self.dirName+'/'+self.theNewestDir)
            self.alarmLevel = 0
            self.alarmLevelActive = False

    def updateNotificationBlockState(self, now):
        if not self.readyToNotify:
            if self.notificationBlockStart is None:
                self.notificationBlockStart = now
            elif now - self.notificationBlockStart >= self.notificationBlockDuration:
                Logger.DEBUG(f"{self.cameraName}: 1 min")
                self.notificationBlockStart = None
                self.readyToNotify = True

    def analyzeMoving(self) -> CameraAnalysisResult | None:
            now = time.monotonic()
            self.updateNotificationBlockState(now)
            smsData = None
            newTheNewestDir = self.getTheNewestDayDir(self.dirName)

            if(newTheNewestDir == 0):
                Logger.ERROR("Error with Disk")
                return CameraAnalysisResult(
                    cameraName=self.cameraName,
                    movementLevel=0,
                    firstDetection=False,
                    message="",
                    reasons="",
                    hasReasons=False,
                    readyToNotify=self.readyToNotify,
                    error="ERROR with Disk",
                )

            if (newTheNewestDir != self.theNewestDir):  #new directory -> new day
                self.countFiles = 0
                self.theNewestDir=newTheNewestDir
            dirOfPhotos = self.dirName+'/'+self.theNewestDir
            newCountFiles = self.getListOfFiles(dirOfPhotos)
            Logger.DEBUG("old count of files:", self.countFiles)
            Logger.DEBUG("new count of files:", newCountFiles)

            firstDetection = False
            if(newCountFiles - self.countFiles >= self.minNewFilesToDetect):
                firstDetection = not self.alarmLevelActive
                self.alarmLevelActive = True
                if not self.readyToNotify:
                    info = "ALARM " + self.cameraName + "- log level +1"
                    self.alarmLevel+=1
                    self.alarmLog(info)

            if self.readyToNotify and self.alarmLevelActive:
                self.alarmLevelActive = False
                if self.alarmLevel == 0:
                    info="ALARM " + self.cameraName
                elif self.alarmLevel <= 1:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - bardzo maly ruch, mogl to byc kot"
                elif self.alarmLevel <= 4:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - ktos nadal sie wluczy po podworku, sprawdz zdjecia"
                elif self.alarmLevel > 4:
                    info="ALARM " + self.cameraName + " - POZIOM " + str(self.alarmLevel) + " - robisz impreze, czy co ? bardzo duzy ruch"

                subDir = self.getTheNewestDayDir(os.path.join(dirOfPhotos, "001", "jpg"))
                subSubDir = self.getTheNewestDayDir(os.path.join(dirOfPhotos, "001", "jpg", subDir))
                dirToFind = os.path.join(dirOfPhotos, "001", "jpg", subDir, subSubDir)
                results = DetectObjects.analyzeMinuteDir(dirToFind, 2)
                tempReasons = ""
                for res in results:
                    if len(res.reasons) > 0:
                        tempReasons += "Detected: "
                        for x in res.reasons:
                            tempReasons += str(x)
                            tempReasons += " "
                if len(tempReasons) > 0:
                    info += " "
                    info += tempReasons
                Logger.INFO(info)

                smsData = CameraAnalysisResult(
                    cameraName=self.cameraName,
                    movementLevel=self.alarmLevel,
                    firstDetection=firstDetection,
                    message=info,
                    reasons=tempReasons,
                    hasReasons=bool(tempReasons),
                    readyToNotify=self.readyToNotify,
                )

                self.readyToNotify = False
                self.notificationBlockStart = now
                self.alarmLevel = 0

                self.alarmLog(info)
            self.countFiles=newCountFiles
            return smsData

    def getTheNewestDayDir(self, dirName):
        if os.path.isdir(dirName):
            dirs = [d for d in os.listdir(dirName)]
            if "DVRWorkDirectory" in dirs:
                dirs.remove('DVRWorkDirectory')
            if(len(dirs)==0):
                Logger.WARNING("Directory", dirName, "is empty")
                return None
            latest_dir=max(dirs, key=os.path.basename)
            return latest_dir
        else:
            Logger.ERROR("Directory",dirName, "doesn't exist")
            return None

    def getListOfFiles(self, dirName):
        if os.path.isdir(dirName):
            listOfFiles = list()
            for (dirpath, dirnames, filenames) in os.walk(dirName):
                listOfFiles += [os.path.join(dirpath, f) for f in os.listdir(dirpath) if f.endswith('.jpg')]
            return len(listOfFiles)
        else:
            return None

    def alarmLog(self, info):
        date_time = datetime.datetime.now().strftime("%Y/%m/%d, %H:%M:%S")
        answer = date_time + ": " + info + '\n'
        file = open(self.logFile,'a')
        file.writelines(answer)
        file.close()
