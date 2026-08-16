import os
import time
import datetime
from dataclasses import dataclass
from Logger import Logger
import DetectObjects

@dataclass
class CameraAnalysisResult:
    message: str
    reasons: str
    hasReasons: bool
    level: int


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

        self.alarmLevelCalculateStartTimestamp = None
        self.readyToNotify = False
        self.isAlarmLevelCalculateFinish = False
        
        self.theNewestDir = self.getTheNewestDayDir(self.dirName)
        if(self.theNewestDir == 0):
            Logger.ERROR("Error with Disk")
        else:
            self.countFiles = 0
            self.compute_added_files(self.theNewestDir)
            self.alarmLevel = 0
            self.isActiveAlarmLevelCalculate = False

    def updateNotificationBlockState(self, now):
        if self.alarmLevelCalculateStartTimestamp is not None:
            if now - self.alarmLevelCalculateStartTimestamp >= self.notificationBlockDuration:
                Logger.DEBUG(f"{self.cameraName}: timeout {self.notificationBlockDuration}")
                self.alarmLevelCalculateStartTimestamp = None
                self.isAlarmLevelCalculateFinish = True

    def analyzeMoving(self) -> CameraAnalysisResult | None:
            result = None
            newTheNewestDir = self.getTheNewestDayDir(self.dirName)
            if(newTheNewestDir == 0):
                #TODO send sms about it
                Logger.ERROR("Error with Disk")
                return CameraAnalysisResult(
                    message="Error with Disk",
                    reasons="Error with Disk",
                    hasReasons=True,
                )
            
            now = time.monotonic()
            self.updateNotificationBlockState(now)

            addedFiles = self.compute_added_files(newTheNewestDir)

            # If the alarm level calculation has not started and the number of added files meets the threshold
            # or the previous calculation has finished, start the alarm level calculation.
            if (self.alarmLevelCalculateStartTimestamp is None and 
                    (self.isAlarmLevelCalculateFinish or addedFiles >= self.minNewFilesToDetect)):
                self.alarmLevel += addedFiles

                info = "ALARM " + self.cameraName
                if self.isAlarmLevelCalculateFinish:
                    self.isAlarmLevelCalculateFinish = False
                    info += " - alarm level calculation finished, log level " + str(self.alarmLevel)
                if self.alarmLevel >= self.minNewFilesToDetect:
                    self.alarmLevelCalculateStartTimestamp = now
                    self.readyToNotify = True
                    info += " - start notification block for " + str(self.notificationBlockDuration) + " sec"
                else:
                    info += " clear alarm level"
                    self.alarmLevel = 0
 
                Logger.INFO(info)
                self.alarmLog(info)
            # If the alarm level calculation has started, update the alarm level and log the information.
            elif self.alarmLevelCalculateStartTimestamp is not None and addedFiles > 0:
                info = "ALARM " + self.cameraName + "- log level +" + str(addedFiles)
                self.alarmLevel += addedFiles
                self.alarmLog(info)
                Logger.INFO(info)
               

            if self.readyToNotify:
                self.readyToNotify = False

                info = f"ALARM {self.cameraName}"
                level = round((self.alarmLevel / int(self.notificationBlockDuration)) * 10)
                if level > 0:
                    info += f" - level {level}/10"
                Logger.DEBUG(f"{self.cameraName}: alarmLevel: {self.alarmLevel}, notificationBlockDuration: {int(self.notificationBlockDuration)}, level: {level}")

                dirOfPhotos = os.path.join(self.dirName, self.theNewestDir)
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

                result = CameraAnalysisResult(
                    message=info,
                    reasons=tempReasons,
                    hasReasons=bool(tempReasons),
                    level=level
                )

                self.alarmLevel = 0

                self.alarmLog(info)
            # state already updated inside compute_added_files
            return result

    def compute_added_files(self, newTheNewestDir):
        """
        Compute number of newly added files since last check and update
        `self.theNewestDir` and `self.countFiles` accordingly.

        Returns:
            addedFiles
        """
        addedFiles = 0
        newCountFiles = 0
        if (newTheNewestDir != self.theNewestDir):  # new directory -> new day
            prevDir = self.theNewestDir
            prevCount = self.countFiles if hasattr(self, 'countFiles') and self.countFiles is not None else 0

            # count new files in previous dir (if exists)
            newCountPrev = 0
            if prevDir:
                prevPath = os.path.join(self.dirName, prevDir)
                c = self.getListOfFiles(prevPath)
                newCountPrev = c if c is not None else 0

            # count files in the new directory
            newPath = os.path.join(self.dirName, newTheNewestDir)
            newCountNew = self.getListOfFiles(newPath)
            newCountNew = newCountNew if newCountNew is not None else 0

            # newly added files are those added to previous dir since last check + all files in new dir
            addedFiles = max(0, newCountPrev - prevCount) + newCountNew

            # update to track the new newest dir counts going forward
            self.theNewestDir = newTheNewestDir
            self.countFiles = newCountNew
            newCountFiles = newCountNew
            Logger.DEBUG(self.cameraName,
                         " count of files in new dir:", newCountNew, 
                         " old count of files:", prevCount, 
                         " new count of files in prev:",  newCountPrev, 
                         " added files:", addedFiles)
        else:
            dirOfPhotos = os.path.join(self.dirName, self.theNewestDir)
            nc = self.getListOfFiles(dirOfPhotos)
            newCountFiles = nc if nc is not None else 0
            addedFiles = newCountFiles - (self.countFiles if self.countFiles is not None else 0)
            # update stored count
            self.countFiles = newCountFiles
            Logger.DEBUG(self.cameraName, 
                         " old count of files:", self.countFiles, 
                         " new count of files:", newCountFiles, 
                         " added files:", addedFiles)

        return addedFiles

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
