import datetime
import inspect
from enum import Enum
from pydoc import ispackage

class bcolors:
    WARNING = '\033[93m'
    ERROR = '\033[91m'
    ENDC = '\033[0m'

class LogLevel(Enum):
    DEBUG = 1
    INFO = 2
    WARNING = 3
    ERROR = 4

class SimpleLogger():
    def __init__(self):
        self.settings()

    def DEBUG(self, *args):
        if self.logLevel.value <= LogLevel.DEBUG.value:
            log = self.getLog("DEBUG", args)
            self.writeToFile(log)
            if self.isPrinting:
                print(log)

    def INFO(self, *args):
        if self.logLevel.value <= LogLevel.INFO.value:
            log = self.getLog("INFO", args)
            self.writeToFile(log)
            if self.isPrinting:
                print(log)

    def WARNING(self, *args):
        if self.logLevel.value <= LogLevel.WARNING.value:
            log = self.getLog("WARNING", args)
            self.writeToFile(log)
            if self.isPrinting:
                print(bcolors.WARNING + log, bcolors.ENDC)

    def ERROR(self, *args):        
        if self.logLevel.value <= LogLevel.ERROR.value:
            log = self.getLog("ERROR", args)
            self.writeToFile(log)
            if self.isPrinting:
                print(bcolors.ERROR + log, bcolors.ENDC)

    def getLog(self, level, args):
        log = self.getTime()
        if self.showFilename:
            log += " %s\t"%(self.getFilename())
        if self.showLogLevel:
            log += " %s:"%(level)
        log += " "
        log += self.getLogText(args)
        return log

    def getLogText(self, args):
        log = ""
        for x in args:
            log += x
            log += " "
        return log

    def getTime(self):
        now = datetime.datetime.now()
        date_time = now.strftime("%Y/%m/%d %H:%M:%S")
        return date_time

    def getFilename(self):
        fullFilenameSplitted = inspect.stack()[3].filename.split('/')
        lineNumber = inspect.stack()[3].lineno
        filename = fullFilenameSplitted[len(fullFilenameSplitted)-1]
        result = "%s:%s"%(filename, lineNumber)
        return result

    def writeToFile(self, log):
        if(self.isSaveToFileEnable):
            f = open(self.fileNameWithPath,"a")
            f.write(log)
            f.write("\n")
            f.close()

    def settings(self, logLevel = LogLevel.INFO, showFilename=True, showLogLevel=True, print=True, saveToFile=False, fileNameWihPath="logger.log"):
        self.isSaveToFileEnable = saveToFile
        self.fileNameWithPath = fileNameWihPath
        self.showFilename = showFilename
        self.showLogLevel = showLogLevel
        self.logLevel = logLevel
        self.isPrinting = print
        if not saveToFile and not print:
            print(bcolors.ERROR + "printing and saving to file is DISABLE !!!", bcolors.ENDC)

Logger = SimpleLogger()
