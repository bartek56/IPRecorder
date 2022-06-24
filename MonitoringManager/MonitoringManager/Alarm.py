import os
import datetime

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

