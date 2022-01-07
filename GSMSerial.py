import serial
import time
import os
import datetime
import urllib
import sh

ser = serial.Serial('/dev/ttyAMA0',19200)  # open serial port
dirNameBrama = '/sharedfolders/MONITORING/brama_cam'
dirNameAltanka = '/sharedfolders/MONITORING/altanka_cam'

def alarmLog(info):
    date_time = datetime.datetime.now().strftime("%Y/%m/%d, %H:%M:%S")
    answer = date_time + ": " + info + '\n'
    file = open("/var/log/Alarm.log",'a')
    file.writelines(answer)
    file.close()

def GSMLog(info):
    date_time = datetime.datetime.now().strftime("%Y/%m/%d, %H:%M:%S")
    answer = date_time + ": " + info + '\n'
    file = open("/var/log/GSMSerial.log",'a')
    file.writelines(answer)
    file.close()
    

def sendSMS(number, text):
    commend='AT+CMGS="' + str(number) + '"\r\n'
    ser.write(commend.encode())
    while ser.inWaiting()<=10:
        time.sleep(0.04)
    time.sleep(0.2)
    data_str = ser.read(ser.inWaiting())
    if ('>' in data_str):
        ser.write(text.encode()) #\x1a
        ser.write(b'\x1a')

def sendSMSAdmin(text):
    sendSMS(999999999, text)

def sendSMSUser(text):
    sendSMS(999999999, text)

def getTheNewestDayDir(dirName):
    if os.path.isdir(dirName):
        dirs = [d for d in os.listdir(dirName)]
        dirs.remove('DVRWorkDirectory')
        latest_dir=max(dirs, key=os.path.basename)
        return latest_dir
    else:
        GSMLog("error with Disk")
        sendSMSAdmin("Error with Disk")
        exit()

#    sub_dirs = [d for d in os.listdir('/sharedfolders/MONITORING/brama_cam/'+latest_dir+'/001/jpg/')]
#    latest_dir_hours=max(sub_dirs, key=os.path.basename)
#    print latest_dir_ho

def getListOfFiles(dirName):
    if os.path.isdir(dirName):
        listOfFiles = list()
        for (dirpath, dirnames, filenames) in os.walk(dirName):
            listOfFiles += [os.path.join(dirpath, f) for f in os.listdir(dirpath) if f.endswith('.jpg')]
        return len(listOfFiles)
    else:
        GSMLog("error with Disk")
        sendSMSAdmin("Error with Disk")
        exit()


def initGSM():
    commend1='AT\r\n'
    ser.write(commend1.encode())
    while ser.inWaiting()<=1:
        time.sleep(0.01)
    ser.read(ser.inWaiting())
    time.sleep(0.5)

    commend1='AT+CMGF=1\r\n'
    ser.write(commend1.encode())
    while ser.inWaiting()<=1:
        time.sleep(0.01)
    ser.read(ser.inWaiting())
    time.sleep(0.5)

    commend2='AT+CNMI=1,2,0,0\r\n'
    ser.write(commend2.encode())
    while ser.inWaiting()<=1:
        time.sleep(0.01)
    ser.read(ser.inWaiting())
    time.sleep(0.5)

    commend3='AT+CLIP=1\r\n'
    ser.write(commend3.encode())
    while ser.inWaiting()<=1:
        time.sleep(0.01)
    time.sleep(0.5)

def checkNetwork():
    try:
        urllib.urlopen("http://google.com") #Python 3.x
        return True
    except:
        return False

def checkMemory():
    rootMemory = sh.awk( sh.grep(sh.df("-h"), "root"), "{print $3 \"/\" $2}")
    diskMemory = sh.awk( sh.grep(sh.df("-h"), "boot/efi"), "{print $3 \"/\" $2}")
    rootMemory = str(rootMemory).strip()
    diskMemory = str(diskMemory).strip()
    return ("External Memory: " + diskMemory + "\n" + "Internal Memory: " + rootMemory)

def checkStatus():
    info = ""
    if not os.path.isdir(dirNameBrama):
        info += "disk is not mounted"
    if not checkNetwork():
        if len(info) > 3:
            info += "\n"
        info += "I'm not connected to network \n" + checkMemory()
    
    if len(info) < 3:
        return "everything okay \n" + checkMemory()
    else:
        return info
    

def main():
    ALARMON = False
    time.sleep(3)
    initGSM()
    time.sleep(3)
    
    counter=0
    counterAltanka=0
    counterBrama=0
    countFilesBrama=0
    countFilesAltanka=0
    theNewestDirAltanka = getTheNewestDayDir(dirNameAltanka)
    theNewestDirBrama = getTheNewestDayDir(dirNameBrama)
    countFilesBrama = getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)
    countFilesAltanka = getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)
    while (True):
    # NB: for PySerial v3.0 or later, use property `in_waiting` instead of function `inWaiting()` below!
        if (ser.inWaiting()>0): #if incoming bytes are waiting to be read from the serial input buffer
            #data_str = ser.read(ser.inWaiting()).decode('ascii') #read the bytes and convert from binary array to ASCII
            data_str = ser.read(ser.inWaiting())
            GSMLog(data_str)
            if('ALARMOFF' in data_str):
                ALARMON=False
                sendSMSUser("Wylaczono ALARM")
            elif('ALARMON' in data_str):
                ALARMON=True
                sendSMSUser("Wlaczono ALARM")
            elif('STATUS' in data_str):
                status = checkStatus()
                sendSMSAdmin(status)
            elif('+CLIP:' in data_str):
                sendSMSAdmin("somebody call to me: \n" + data_str)
#            elif('AT+' not in data_str):
#                sendSMSAdmin("I don't understand \n" + data_str)


        if (counter > 20):
            counter=0
            newTheNewestDirAltanka = getTheNewestDayDir(dirNameAltanka)
            if (newTheNewestDirAltanka!= theNewestDirAltanka):  #new directory -> new day
                countFilesAltanka = 0
                theNewestDirAltanka=newTheNewestDirAltanka
            newCountFilesAltanka = getListOfFiles(dirNameAltanka+'/'+theNewestDirAltanka)
            if(newCountFilesAltanka-countFilesAltanka==2): # come in to loop, when somebody come in to possesion
                time.sleep(1.5)

            if(newCountFilesAltanka-countFilesAltanka>2):
                info = "ALARM ALTANKA"
                if(ALARMON==True):
                    sendSMSUser(info)
                alarmLog(info)
            countFilesAltanka=newCountFilesAltanka

            newTheNewestDirBrama = getTheNewestDayDir(dirNameBrama)
            if (newTheNewestDirBrama != theNewestDirBrama):
                countFilesBrama = 0
                theNewestDirBrama=newTheNewestDirBrama
            newCountFilesBrama = getListOfFiles(dirNameBrama+'/'+theNewestDirBrama)

            if(newCountFilesBrama-countFilesBrama>2):
                info = "ALARM BRAMA"
                if(ALARMON==True):
                    sendSMSUser(info)
                alarmLog(info)
            countFilesBrama=newCountFilesBrama
        
        counter=counter+1
       
        if(os.path.exists("/etc/scripts/SMS.txt")):
            file = open("/etc/scripts/SMS.txt","r")
            text = file.read()
            file.close()
            sendSMSAdmin(text)
            os.remove("/etc/scripts/SMS.txt")
        
        time.sleep(0.5)

if __name__ == '__main__':
    main()

