from enum import Enum
import os
from Logger import Logger


class NumberType(Enum):
    MOBILE = "MOBILE"
    HOME = "HOME"
    HOME_FAX = "HOME_FAX"
    WORK = "WORK"
    WORK_FAX = "WORK_FAX"
    WORK_MOBILE = "WORK_MOBILE"

class Number():
    def __init__(self, number="", type=NumberType.MOBILE):
        self.type = type
        self.number = number

class EmailType(Enum):
    HOME = "HOME"
    WORK = "WORK"
    OTHER = "OTHER"

class Email():
    def __init__(self, email="", type=EmailType.WORK):
        self.type = type
        self.email = email

class Contact():
    def __init__(self, name="", surname="", emails=[], numbers=[]):
        self.name = name
        self.surname = surname
        self.numbers = numbers
        self.emails = emails

class Contacts():
    def __init__(self, fileName="/etc/scripts/contacts.txt"):
        self.tel = []
        self.fileName = fileName
        self.readContacts()

    def readContacts(self):
        if os.path.isfile(self.fileName):
            f = open(self.fileName, "r")
            lines = f.readlines()
            f.close()
            for x in lines:
                x = x.strip()
                contact = x.split(" ")
                #contact[len(contact)-1].strip()
                numbers = []
                emails = []

                for dataIndex in range(2, len(contact)):
                    if "@" in contact[dataIndex]:
                        emailWithType = contact[dataIndex].split(":")
                        email = Email()
                        email.type = EmailType(emailWithType[0])                        
                        email.email = emailWithType[1]
                        emails.append(email)
                    else:
                        number = Number()
                        numberWithType = contact[dataIndex].split(":")
                        number.type = NumberType(numberWithType[0])
                        number.number = numberWithType[1]
                        numbers.append(number)

                self.tel.append(Contact(contact[0], contact[1], emails, numbers))
        else:
            Logger.WARNING("File with contacts:", self.fileName, "does not exist!")

    def SaveToFile(self):
        f = open(self.fileName, "w")
        for contact in self.tel:
            numbersString = ""
            numbersLen = len(contact.numbers)
            for x in range(numbersLen):
                numbersString += contact.numbers[x].type.name + ":" + contact.numbers[x].number
                if x != numbersLen-1:
                    numbersString += " "

            emailsString = ""
            emailsLen = len(contact.emails)
            for x in range(emailsLen):
                emailsString += contact.emails[x].type.name + ":" + contact.emails[x].email
                if x != emailsLen-1:
                    emailsString += " "

            contactStr = "%s %s %s %s"%(contact.name, contact.surname, emailsString, numbersString)

            f.write(contactStr)
            f.write("\n")
        f.close()

    def ShowContacts(self):
        print("\t******************")
        for contact in self.tel:
            numbers = ""
            numbersLen = len(contact.numbers)
            for x in range(numbersLen):
                numbers += contact.numbers[x].type.name + ":" + contact.numbers[x].number
                if x != numbersLen:
                    numbers += " "

            tempInfo = contact.name + " " + contact.surname + " " + numbers
            print(tempInfo) 
        print("\t******************")    

    def AddContact(self, name, surname, *data):                 
        for newNumberOrEmailStruct in data:
            if isinstance(newNumberOrEmailStruct, Number):
                if len(newNumberOrEmailStruct.number) != 9:
                    Logger.WARNING("wrong number")
                    return 
            if isinstance(newNumberOrEmailStruct, Email):
                if  "@" not in newNumberOrEmailStruct.email:
                    Logger.WARNING("wrong mail")
                    return
        for contact in self.tel:
            if contact.name == name and contact.surname == surname:
                Logger.WARNING("Contact", contact.name, contact.surname , "already exist")
                return

        newNumbersList = []
        newEmailList = []
        for newNumberOrEmail in data:
            if isinstance(newNumberOrEmail, Number):
                newNumbersList.append(newNumberOrEmail)
            if isinstance(newNumberOrEmail, Email):
                newEmailList.append(newNumberOrEmail)    

        contact = Contact(name,surname, newEmailList, newNumbersList)
        self.tel.append(contact)
    
    def RemoveContact(self, name, surname):
        index = 0
        for x in self.tel:
            if x.name == name and x.surname == surname:
                self.tel.pop(index)
                return True
            index += 1     
        return False        

    def SetSortingByName(self):
        def takeName(elem):
            return elem.name
        self.tel.sort(key=takeName)

    def SetSortingBySurname(self):
        self.tel.sort(key=lambda x:x.surname )

    # TODO
    def ShowContactsSortedByName(self):
        self.ShowContacts()
    
    # TODO
    def ShowContactsSortedBySurname(self):
        self.ShowContacts()     

    def LookingForContacts(self, nameOrSurname):
        contacts = []
        nameOrSurnameCheck = nameOrSurname.split(" ")

        if len(nameOrSurnameCheck)==2:
            name = nameOrSurnameCheck[0]
            surname = nameOrSurnameCheck[1]
            for x in self.LookingForContactByNameAndSurname(name,surname):
                contacts.append(x)
        else:
            for x in self.LookingForContactByName(nameOrSurname):
                contacts.append(x)
            for x in self.LookingForContactBySurName(nameOrSurname):
                contacts.append(x)
        
        return contacts        

    def LookingForContact(self, nameOrSurname):
        contacts = self.LookingForContacts(nameOrSurname)
        if len(contacts) ==1:
            return contacts[0]
        else:
            return 

    def LookingForContactBySurName(self, surname):
        for contact in self.tel:
            if surname in contact.surname:
                yield Contact(contact.name, contact.surname, contact.emails, contact.numbers)

    def LookingForContactByName(self, name):
        for contact in self.tel:
            if name in contact.name:
                yield Contact(contact.name, contact.surname, contact.emails, contact.numbers)

    def LookingForContactByNameAndSurname(self, name, surname):
        for contact in self.tel:
            if name in contact.name and surname in contact.surname:
                yield Contact(contact.name, contact.surname, contact.emails, contact.numbers)

    def LookingForContactByNumber(self, number):
        for contact in self.tel:
            for numberStruct in contact.numbers:
                if numberStruct.number == number:
                    return Contact(contact.name, contact.surname, contact.emails, contact.numbers)

    def AddNumbers(self, name, surname, *numbers):
        for newNumberStruct in numbers:
            for contact in self.tel:
                for numberStruct in contact.numbers:
                    if newNumberStruct.number == numberStruct.number:
                        Logger.WARNING("Contact:", contact.name, contact.surname ,"with number", numberStruct.number, "already exist")
                        return
            if len(newNumberStruct.number) != 9:
                Logger.WARNING("wrong number")
                return 

        for contact in self.tel:
            if contact.name == name and contact.surname == surname:
                for newNumberStruct in numbers:
                    contact.numbers.append(newNumberStruct)
    
    def RemoveNumbers(self, name, surname, *numbers):
        for index in range(len(self.tel)):
            if self.tel[index].name == name and self.tel[index].surname == surname:
                numbersStructToRemove = []
                removed = False
                for numberStruct in self.tel[index].numbers:
                    if numberStruct.number in numbers:
                        numbersStructToRemove.append(numberStruct)
                        removed = True

                for structrm in numbersStructToRemove:
                    self.tel[index].numbers.remove(structrm)
                    
                       
                return removed
        return False        

    def GetNumbers(self, nameOrSurname):
        numbers = []
        contact = self.LookingForContact(nameOrSurname)
        if contact == None:
            Logger.WARNING("contact",nameOrSurname, "does not exist")
            return
        else:
            for x in contact.numbers:
                numbers.append(x.number)
            return numbers
    
    def GetDefaultNumber(self, nameOrSurname):
        numbers = self.GetNumbers(nameOrSurname)
        if numbers != None and len(numbers)>0:
            return numbers[0]

    def setDefaultNumber(self, nameOrSurname, defaultNumber):
        contact = self.LookingForContact(nameOrSurname)
        if contact is None:
            Logger.WARNING("I can not find contact")
            return False

        for index in range(len(self.tel)):
            if self.tel[index].name == contact.name and self.tel[index].surname == contact.surname:
                numbers = self.tel[index].numbers
                numberExist = False
                numberIndex=0

                for number in numbers:
                    if number.number == defaultNumber:
                        numberExist = True
                        break
                    numberIndex+=1

                if not numberExist:
                    Logger.WARNING("wrong number to set as default")
                    return False

                numbers.insert(0, numbers.pop(numberIndex))
                return True

    def AddEmails(self, name, surname, *emails):
        for newEmailStruct in emails:
            for contact in self.tel:
                for emailStruct in contact.emails:
                    if newEmailStruct.email == emailStruct.email:
                        Logger.WARNING("Contact:", contact.name, contact.surname ,"with email", emailStruct.email, "already exist")
                        return
            if "@" not in newEmailStruct.email:
                Logger.WARNING("wrong email")
                return 

        for contact in self.tel:
            if contact.name == name and contact.surname == surname:
                for newEmailStruct in emails:
                    contact.emails.append(newEmailStruct)

    def RemoveEmails(self, name, surname, *emails):
        for index in range(len(self.tel)):
            if self.tel[index].name == name and self.tel[index].surname == surname:
                emailsStructToRemove = []
                removed = False
                for emailStruct in self.tel[index].emails:
                    if emailStruct.email in emails:
                        emailsStructToRemove.append(emailStruct)
                        removed = True

                for structrm in emailsStructToRemove:
                    self.tel[index].emails.remove(structrm)
                       
                return removed
        return False        

    def GetEmails(self, nameOrSurname):
        emails = []
        contact = self.LookingForContact(nameOrSurname)
        if contact == None:
            Logger.WARNING("contact",nameOrSurname, "does not exist")
            return
        else:
            for x in contact.emails:
                emails.append(x.email)
            return emails
    
    def GetDefaultEmail(self, nameOrSurname):
        emails = self.GetEmails(nameOrSurname)
        if emails != None and len(emails)>0:
            return emails[0]
     
    def setDefaultEmail(self, nameOrSurname, defaultEmail):
        contact = self.LookingForContact(nameOrSurname)
        if contact is None:
            Logger.WARNING("I can not find contact")
            return False

        for index in range(len(self.tel)):
            if self.tel[index].name == contact.name and self.tel[index].surname == contact.surname:
                emails = self.tel[index].emails
                emailExist = False
                emailIndex=0

                for email in emails:
                    if email.email == defaultEmail:
                        emailExist = True
                        break
                    emailIndex+=1

                if not emailExist:
                    Logger.WARNING("wrong number to set as default")
                    return False

                emails.insert(0, emails.pop(emailIndex))
                return True

if __name__ == "__main__":
    phone = Contacts()
    phone.AddContact("Krzysztof", "Kowalczyk", Email(email="Krzysztof.Kowalczyk@gmail.com"),
                                              Number(number="123456789", type=NumberType.MOBILE)
                                              )
    phone.SaveToFile()
