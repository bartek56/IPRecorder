import os
import time

from Contacts import Contacts
from GSMSerial import GSMSerial, IpRecorderStatus
#from mailManager import Mail
from Logger import Logger


class ActiveUser():
    def __init__(self, name="", surname=""):
        self.name = name
        self.surname = surname

class NotificationManager():
    def __init__(self, activeUsersFile="/etc/scripts/active_users.txt"):
        self.adminNumber = "123456789"
        self.ipRecorderStatus = IpRecorderStatus()
#        self.mailManager = Mail()
        self.phoneContacts = Contacts("/etc/scripts/contacts.txt")
        self.readyToSMS = False
        self.gsmSerial = GSMSerial()
        time.sleep(3)
        self.readyToSMS = True
        self.usersList = []
        self.fileName = activeUsersFile
        if os.path.isfile(self.fileName):
            file = open(self.fileName, "r")
            lines = file.readlines()
            file.close()
            for user in lines:
                if len(user)>2:
                    userNameAndSurname = user.strip()
                    userNameAndSurnameList = userNameAndSurname.split(" ")
                    name = userNameAndSurnameList[0]
                    surname = userNameAndSurnameList[1]
                    self.usersList.append(ActiveUser(name, surname))
    
    def showActiveContacts(self):
        for x in self.usersList:
            print(x.name, x.surname)

    def addUserAsActive(self, name, surname):
        for x in self.usersList:
            if x.name == name and x.surname == surname:
                Logger.WARNING("Contact exist in active users")
                return 

        contactExist = False
        for x in self.phoneContacts.tel:
            if x.name == name and x.surname == surname:
                contactExist = True
        if not contactExist:
            Logger.WARNING("Contact", name, surname, "not exist")
            return 
           
        self.usersList.append(ActiveUser(name, surname))
        Logger.INFO("added contact as active:", name, surname)

    def removeUserFromActive(self, name, surname):
        index = 0
        contactExist = False
        for x in self.usersList:
            if x.name == name and x.surname == surname:
                contactExist = True 
                break
            index+=1    
        
        if(contactExist):
            Logger.INFO("Contact", name, surname, " removed from active list")
            self.usersList.pop(index)
        else:
            Logger.WARNING("Contact", name, surname, "not exist in active list")

    def saveToFile(self):
        fileToSave = open(self.fileName, "w")

        for user in self.usersList:
            fileToSave.write(user.name)
            fileToSave.write(" ")
            fileToSave.write(user.surname)
            fileToSave.write("\n")

        self.phoneContacts.SaveToFile()    

#    def sendMailNotification(self, subject, message):        
#        for x in self.usersList:
#            nameAndSurname = x.name + " " + x.surname
#            email = self.phoneContacts.GetDefaultEmail(nameAndSurname)
#            self.mailManager.sendMail(email, subject, message)
    
    def sendSMSNotification(self, message):        
        for x in self.usersList:
            nameAndSurname = x.name + " " + x.surname
            number = self.phoneContacts.GetDefaultNumber(nameAndSurname)
            self.gsmSerial.sendSMS(number, message)

    def sendSMS(self, number, message):        
        self.gsmSerial.sendSMS(number, message)

    def sendSMSAdmin(self, message):        
        self.gsmSerial.sendSMS(self.adminNumber, message)

    def checkSender(self, data_str):
        dataStrTemp = data_str
        dataStrTempSplitted = dataStrTemp.split(",,")
        number = dataStrTempSplitted[0].replace('"', '')
        number = number.replace("+48", "")
        number = number.replace("+CMT: ", "")

        number = number.strip()
        contact = self.phoneContacts.LookingForContactByNumber(number)

        return contact


    def readAT(self):    
        data_str = self.gsmSerial.readMessages()

        if(data_str is not None):
            # text message        
            if('+CMT:' in data_str):
                if('STATUS' in data_str):
                    status = self.ipRecorderStatus.checkStatus()
                    self.sendSMSAdmin(status)

                contact = self.checkSender(data_str)
                if contact is None:
                     self.sendSMSAdmin("unknown number send sms to me")
                     return
                else:                        
                    if('ALARMOFF' in data_str):
                        self.sendSMS(contact.numbers[0].number, "Wylaczono ALARM")
                        self.removeUserFromActive(contact.name, contact.surname)
                    elif('ALARMON' in data_str):
                        self.sendSMS(contact.numbers[0].number, "Wlaczono ALARM")
                        self.addUserAsActive(contact.name, contact.surname)
            # calling           
            elif('+CLIP:' in data_str):
                self.sendSMSAdmin("somebody call to me: \n" + data_str)
            # send SMS    
            elif('+CMGS:' in data_str):
                self.readyToSMS=True
            # error
            elif('ERROR' in data_str):
                self.readyToSMS=True
            #elif('AT+' not in data_str):
                #sendSMSAdmin("I don't understand \n" + data_str)


#if __name__ == "__main__":
#    notificationManager = NotificationManager()
#    notificationManager.sendMailNotification("IPRecorder","Hello World")
