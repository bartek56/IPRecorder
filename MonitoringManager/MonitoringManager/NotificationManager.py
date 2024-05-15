import os
import time

from Contacts import Contacts
from StatusManager import IpRecorderStatus
# from mailManager import Mail
from Logger import Logger
import libGSMEngine as GSMSerial


class ActiveUser():
    def __init__(self, name="", surname=""):
        self.name = name
        self.surname = surname


class NotificationManager():
    def __init__(self, activeUsersFile="/etc/scripts/active_users.txt"):
        self.adminNumber = "987654321"
        self.ipRecorderStatus = IpRecorderStatus()
#        self.mailManager = Mail()
        self.phoneContacts = Contacts("/etc/scripts/contacts.txt")
        self.readyToSMS = False
        self.gsmManager = GSMSerial.GSMManager("/dev/ttyAMA0")
        self.gsmManager.initialize()
        time.sleep(3)
        self.readyToSMS = True
        self.usersList = []
        self.fileName = activeUsersFile
        if os.path.isfile(self.fileName):
            file = open(self.fileName, "r")
            lines = file.readlines()
            file.close()
            for user in lines:
                if len(user) > 2:
                    userNameAndSurname = user.strip()
                    userNameAndSurnameList = userNameAndSurname.split(" ")
                    name = userNameAndSurnameList[0]
                    surname = userNameAndSurnameList[1]
                    self.usersList.append(ActiveUser(name, surname))

    def showActiveContacts(self):
        for x in self.usersList:
            print(x.name, x.surname)

    def getActiveContacts(self):
        activeContact = []
        for x in self.usersList:
            activeContact.append(x.name + " " + x.surname)
        return activeContact

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
            index += 1

        if (contactExist):
            Logger.INFO("Contact", name, surname, " removed from active list")
            self.usersList.pop(index)
        else:
            Logger.WARNING("Contact", name,
                           surname, "not exist in active list")

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
            self.gsmManager.sendSms("+48"+number, message)

    def sendSMS(self, number, message):
        self.gsmManager.sendSms("+48"+number, message) # TODO

    def sendSMSAdmin(self, message):
        self.gsmManager.sendSms("+48"+self.adminNumber, message) # TODO

    def checkStatus(self):
        status = self.ipRecorderStatus.checkStatus()
        activeContacts = self.getActiveContacts()
        if len(activeContacts) > 0:
            status += "\nActive Contacts:\n"
            for x in activeContacts:
                status += x
                status += " "
        return status

    def checkSender(self, number):
        number = number.replace("+48", "") # TODO
        contact = self.phoneContacts.LookingForContactByNumber(number)
        return contact

    def checkNewMessage(self):
        if (not self.gsmManager.isNewSms()):
            return
        sms = self.gsmManager.getSms()
        contact = self.checkSender(sms.number)
        if contact is None:
            self.sendSMSAdmin("unknown number send sms to me")
            return
        data_str = sms.msg

        if ('STATUS' in data_str):
            self.sendSMS(contact.numbers[0].number, self.checkStatus())
        elif ('ALARMOFF' in data_str):
            self.sendSMS(contact.numbers[0].number, "Wylaczono ALARM")
            self.removeUserFromActive(contact.name, contact.surname)
        elif ('ALARMON' in data_str):
            self.sendSMS(contact.numbers[0].number, "Wlaczono ALARM")
            self.addUserAsActive(contact.name, contact.surname)
        else:
            self.sendSMS(contact.numbers[0].number,
                         "Nie wiem, czego ode mnie zadasz")


# if __name__ == "__main__":
#    notificationManager = NotificationManager()
#    notificationManager.sendMailNotification("IPRecorder","Hello World")
