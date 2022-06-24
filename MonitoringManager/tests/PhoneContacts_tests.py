import unittest
from unittest import TestCase
import os
from PhoneContacts.PhoneContacts import Contacts
from PhoneContacts.PhoneContacts import Number
from PhoneContacts.PhoneContacts import Email

class PhoneContactsTests(TestCase):
    @classmethod
    def setUpClass(cls):
        cls._fileName = "contactsTests.txt"
        print("setUpClass")

    def setUp(self):
        print("setUp")

    def tearDown(self):
        print("tearDown")
    
    @classmethod
    def tearDownClass(cls):
        print("tearDownClass")

    def test_ReadContacts_EmptyContacts(self):
        phone = Contacts(self.__class__._fileName)
        self.assertEqual(len(phone.tel), 0)

    def test_AddContact_TwoContacts(self):
        phone1 = Contacts(self.__class__._fileName)
        name1 = "Jan"
        surname1 = "Nowak"
        email11 = "jan@gmail.com"
        email12 = "nowak@gmail.com"
        number11 = "789456123"
        number12 = "123456789"

        name2 = "Krzysztof"
        surname2 = "Kowalczyk"
        email21 = "krzysztof.kowalczyk@wp.pl"
        number21 = "456789123"


        phone1.AddContact(name1, surname1, Email(email=email11), Email(email=email12), Number(number=number11), Number(number=number12))
        phone1.AddContact(name2, surname2, Email(email=email21), Number(number=number21))
        phone1.SaveToFile()

        phone2 = Contacts(self.__class__._fileName)


        self.assertEqual(len(phone2.tel), 2)

        self.assertEqual(phone2.tel[0].name, name1)
        self.assertEqual(phone2.tel[0].surname, surname1)
        self.assertEqual(phone2.tel[0].emails[0].email, email11)
        self.assertEqual(phone2.tel[0].emails[1].email, email12)
        self.assertEqual(phone2.tel[0].numbers[0].number, number11)
        self.assertEqual(phone2.tel[0].numbers[1].number, number12)

        self.assertEqual(phone2.tel[1].name, name2)
        self.assertEqual(phone2.tel[1].surname, surname2)
        self.assertEqual(phone2.tel[1].numbers[0].number, number21)
        self.assertEqual(phone2.tel[1].emails[0].email, email21)

        os.remove(phone1.fileName)

    def test_AddContact_TwoTheSameContacts(self):
        phone1 = Contacts(self.__class__._fileName)
        name = "Jan"
        surname = "Nowak"
        email = "nowak@gmail.com"

        phone1.AddContact(name, surname, Email(email=email))
        phone1.AddContact(name, surname, Email(email=email))
        phone1.SaveToFile()

        phone2 = Contacts(self.__class__._fileName)        


        self.assertEqual(len(phone2.tel), 1)

        self.assertEqual(phone2.tel[0].name, name)
        self.assertEqual(phone2.tel[0].surname, surname)
        self.assertEqual(phone2.tel[0].emails[0].email, email)

        os.remove(phone1.fileName)

    def test_AddContact_WrongNumber(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="987654321454"))
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456"))

        self.assertEqual(len(phone.tel), 0)

    def test_AddContact_WrongEmail(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email(email="fdgtgrfdgrgfdgf.com") ,Number(number="789456123"))
        self.assertEqual(len(phone.tel), 0)

    def test_RemoveContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456789"))
        phone.AddContact("Krzysztof","Kowalczyk2", Number(number="987654321"))
        self.assertEqual(len(phone.tel), 2)

        phone.RemoveContact("Krzysztof","Kowalczyk2")        

        self.assertEqual(len(phone.tel), 1)

        contact = phone.LookingForContact("Krzysztof Kowalczyk")
        self.assertEqual(contact.name ,"Krzysztof")
        self.assertEqual(contact.surname, "Kowalczyk")
        self.assertEqual(len(contact.numbers), 1)
        self.assertEqual(len(contact.emails), 0)
        
        self.assertEqual(contact.numbers[0].number, "123456789")
    
    def test_setSortingByName(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Aaa","fdfdf")
        phone.AddContact("Zzz","dfdfdf")
        phone.AddContact("Kkk","dfdfdf")
        phone.AddContact("Ddd","dfdfdf")
        
        phone.SetSortingByName()

        self.assertEqual(phone.tel[0].name,"Aaa")
        self.assertEqual(phone.tel[1].name,"Ddd")
        self.assertEqual(phone.tel[2].name,"Kkk")
        self.assertEqual(phone.tel[3].name,"Zzz")

    def test_setSortingBySurName(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("ddfd","Aaa")
        phone.AddContact("dfdfdf","Zzz")
        phone.AddContact("dfdfdf","Kkk")
        phone.AddContact("dfdfdf","Ddd")
        
        phone.SetSortingBySurname()

        self.assertEqual(phone.tel[0].surname,"Aaa")
        self.assertEqual(phone.tel[1].surname,"Ddd")
        self.assertEqual(phone.tel[2].surname,"Kkk")
        self.assertEqual(phone.tel[3].surname,"Zzz")

    def test_LookingForContacts_IfExistOne(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak", Number("789456123"), Number("456789123"))
        phone.AddContact("Krzysztof","Kowalczyk", Number("987654321"), Number("123456789"))

        lookingForName = "Jan"
        foundedContacts = phone.LookingForContacts(lookingForName)
        for contact in foundedContacts:
            self.assertEqual(contact.name, lookingForName)
            self.assertEqual(contact.surname, "Nowak")
            self.assertEqual(len(contact.numbers), 2)
            self.assertEqual(len(contact.emails), 0)

    def test_LookingForContacts_IfExistsTwo(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak", Number("456789123"), Number("789456123"))
        phone.AddContact("Jan","Kowalczyk", Number("987654321"), Number("123456789"))

        lookingForName = "Jan"
        foundedContacts = phone.LookingForContacts(lookingForName)

        self.assertEqual(len(foundedContacts),2)  

    def test_LookingForContact_IfExistOne(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak", Number("789456123"), Number("456789123"))
        phone.AddContact("Krzysztof","Kowalczyk", Number("987654321"), Number("123456789"))

        lookingForName = "Jan"
        contact = phone.LookingForContact(lookingForName)

        self.assertEqual(contact.name, lookingForName)
        self.assertEqual(contact.surname, "Nowak")
        self.assertEqual(contact.numbers[0].number, "789456123")
        self.assertEqual(contact.numbers[1].number, "456789123")
        self.assertEqual(len(contact.numbers), 2)
        self.assertEqual(len(contact.emails), 0)

    def test_LookingForContact_IfExistTwo(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak", Number("456789123"), Number("789456123"))
        phone.AddContact("Jan","Kowalczyk", Number("987654321"), Number("123456789"))

        lookingForName = "Jan"
        contact = phone.LookingForContact(lookingForName)

        self.assertEqual(contact, None)
 
    def test_LookingForContact_ByNameAndSurname_1(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak", Number(number="789456123"), Number(number="456789123"))
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="987654321"), Number(number="123456789"))

        lookingForName = "Jan Nowak"
        contact = phone.LookingForContacts(lookingForName)

        self.assertEqual(len(contact), 1)

    def test_LookingForContact_ByNameAndSurname_2(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak")
        phone.AddContact("Jan","Kowalczyk")

        lookingForName = "Jan"
        contact = phone.LookingForContacts(lookingForName)

        self.assertEqual(len(contact), 2)

    def test_LookingForContact_BySurname(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak")
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="987654321"), Number(number="123456789"))

        lookingForSurName = "Kowalczyk"
        contact = phone.LookingForContacts(lookingForSurName)

        self.assertEqual(len(contact), 1)
        self.assertEqual(contact[0].surname, lookingForSurName)
        self.assertEqual(contact[0].name, "Krzysztof")
        self.assertEqual(contact[0].numbers[0].number, "987654321")
        self.assertEqual(contact[0].numbers[1].number, "123456789")

    def test_LookingForContact_IfExistTwo_1(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Jan","Nowak")
        phone.AddContact("Jan","Kowalczyk")
        lookingForName = "Bartosz"
        contact = phone.LookingForContact(lookingForName)
        self.assertEqual(contact, None)
 
    def test_LookingForContact_IfExistTwo_2(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk")
        phone.AddContact("Jan","Kowalczyk")
        self.assertEqual(len(phone.tel), 2)
        lookingForSurName = "Kowalczyk"
        contact = phone.LookingForContact(lookingForSurName)
        self.assertEqual(contact, None)

    def test_AddNumbers_ToContact(self):
        phone = Contacts(self.__class__._fileName)
        numberList = ["789456123", "456789123", "123456789"]
        phone.AddContact("Krzysztof","Kowalczyk", Number(numberList[0]))

        lookingForName = "Krzysztof"
        self.assertEqual(phone.GetDefaultNumber(lookingForName), numberList[0])

        phone.AddNumbers("Krzysztof", "Kowalczyk", Number(number=numberList[1]), Number(number=numberList[2]))
        index = 0

        numbers = phone.GetNumbers(lookingForName)
        for number in numbers:
            self.assertEqual(number, numberList[index])
            index += 1

        self.assertEqual(index, len(numberList))    
        
        self.assertEqual(phone.GetDefaultNumber(lookingForName), numberList[0])

    def test_RemoveNumbers_FromContact(self):
        phone = Contacts(self.__class__._fileName)
        numberList = ["789456123", "456789123", "123456789"]
        phone.AddContact("Krzysztof","Kowalczyk", Number(numberList[0]), Number(numberList[1]), Number(numberList[2]))

        lookingForName = "Krzysztof"

        index = 0
        numbers = phone.GetNumbers(lookingForName)
        for number in numberList:
            self.assertEqual(number, numbers[index])
            index += 1

        self.assertEqual(index, len(numberList))

        numberToRemove1 = numberList.pop(1)
        numberToRemove2 = numberList.pop(1)

        self.assertTrue(phone.RemoveNumbers("Krzysztof", "Kowalczyk", numberToRemove1, numberToRemove2))

        index = 0
        numbers = phone.GetNumbers(lookingForName)
        for number in numberList:
            self.assertEqual(number, numbers[index])
            index += 1

        self.assertEqual(index, len(numbers))

    def test_RemoveNumber_FromContactIfNumbersNotExist(self):
        phone = Contacts(self.__class__._fileName)
        numberList = ["789456123", "456789123", "123456789"]
        phone.AddContact("Krzysztof","Kowalczyk", Number(numberList[0]), Number(numberList[1]), Number(numberList[2]))

        lookingForName = "Krzysztof"

        numbers = phone.GetNumbers(lookingForName)        
        self.assertEqual(len(numberList), len(numbers))

        self.assertFalse(phone.RemoveNumbers("Bartosz", "Brzozowski", "789456123"))
        
        numbers = phone.GetNumbers(lookingForName)        
        self.assertEqual(len(numberList), len(numbers))

    def test_GetNumbers(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="789456132"), Number(number="456789123"), Number("789456123"))
        numbers = phone.GetNumbers("Krzysztof Kowalczyk")
        self.assertEqual(3, len(numbers))

    def test_GetDefaultNumber_FromContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456789"), Number(number="987654321"), Number("789456123"))

        self.assertEqual("123456789", phone.GetDefaultNumber("Krzysztof Kowalczyk"))
 
    def test_GetDefaultNumber_FromContactIfNotExist(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("test@test.com"))

        self.assertEqual(None, phone.GetDefaultNumber("Krzysztof Kowalczyk"))
 
    def test_SetDefaultNumber_InContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456789"), Number(number="987654321"), Number("789456123"))

        self.assertEqual("123456789", phone.GetDefaultNumber("Krzysztof Kowalczyk"))
        self.assertTrue(phone.setDefaultNumber("Krzysztof Kowalczyk", "789456123"))
        self.assertEqual("789456123", phone.GetDefaultNumber("Krzysztof Kowalczyk"))

    def test_SetDefaultNumber_InContactWrongNumber(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456789"), Number(number="789456123"), Number("987654321"))

        self.assertFalse(phone.setDefaultNumber("Krzysztof Kowalczyk", "789456126"))
        self.assertEqual("123456789", phone.GetDefaultNumber("Krzysztof Kowalczyk"))

    def test_SetDefaultNumber_InContactWrongContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number(number="123456789"), Number(number="456789123"), Number("789456123"))

        self.assertFalse(phone.setDefaultNumber("Krzysztof Nowak", "789456123"))
        self.assertEqual("123456789", phone.GetDefaultNumber("Krzysztof Kowalczyk"))

    def test_AddEmails_ToContact(self):
        phone = Contacts(self.__class__._fileName)
        emailsList = ["abc@gmail.com", "def@gmail.com", "ghi@gmail.com"]
        phone.AddContact("Krzysztof","Kowalczyk", Email(email=emailsList[0]))

        lookingForName = "Krzysztof"
        self.assertEqual(phone.GetDefaultEmail(lookingForName), emailsList[0])

        phone.AddEmails("Krzysztof", "Kowalczyk", Email(email=emailsList[1]), Email(email=emailsList[2]))
        index = 0
        emails = phone.GetEmails(lookingForName)
        for email in emails:
            self.assertEqual(email, emailsList[index])
            index += 1
        self.assertEqual(index, len(emailsList))    
        
    def test_RemoveEmails_FromContact(self):
        phone = Contacts(self.__class__._fileName)
        emailsList = ["abc@gmail.com", "def@gmail.com", "ghi@gmail.com"]

        phone.AddContact("Krzysztof","Kowalczyk", Email(emailsList[0]), Email(email=emailsList[1]), Email(email=emailsList[2]))

        lookingForName = "Krzysztof"

        index = 0
        emails = phone.GetEmails(lookingForName)
        for email in emailsList:
            self.assertEqual(email, emails[index])
            index += 1

        self.assertEqual(index, len(emailsList))

        emailToRemove1 = emailsList.pop(1)
        emailToRemove2 = emailsList.pop(1)

        phone.RemoveEmails("Krzysztof", "Kowalczyk", emailToRemove1, emailToRemove2)

        index = 0
        emails = phone.GetEmails(lookingForName)
        for email in emailsList:
            self.assertEqual(email, emails[index])
            index += 1

        self.assertEqual(index, len(emails))

    def test_RemoveEmails_FromContactIfEmailNotExist(self):
        phone = Contacts(self.__class__._fileName)
        emailsList = ["abc@gmail.com", "def@gmail.com", "ghi@gmail.com"]

        phone.AddContact("Krzysztof", "Kowalczyk", Email(emailsList[0]), Email(email=emailsList[1]), Email(email=emailsList[2]))

        lookingForName = "Krzysztof"

        emails = phone.GetEmails(lookingForName)
        self.assertEqual(len(emailsList), len(emails))


        self.assertFalse(phone.RemoveEmails("Krzysztof", "Kowalczyk", "test@test.com"))

        emails = phone.GetEmails(lookingForName)
        self.assertEqual(len(emailsList), len(emails))
    
    def test_GetEmails(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("test@test.com"), Email("krzysztof.kowalczyk@test.com"), Email("Krzysztow@wp.pl"))
        emails = phone.GetEmails("Krzysztof Kowalczyk")

        self.assertEqual(emails[0], "test@test.com")
        self.assertEqual(emails[1], "krzysztof.kowalczyk@test.com")
        self.assertEqual(emails[2], "Krzysztow@wp.pl")      
        

    def test_GetDefaultEmail_FromContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("test@test.com"), Email("krzysztof.kowalczyk@test.com"), Email("Krzysztow@wp.pl"))

        self.assertEqual("test@test.com", phone.GetDefaultEmail("Krzysztof Kowalczyk"))
 
    def test_GetDefaultEmail_FromContactIfNotExist(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number("789456123"))

        self.assertEqual(None, phone.GetDefaultEmail("Krzysztof Kowalczyk"))
 
    def test_GetDefaultEmail_FromContactIfNotExistContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Number("789456123"))

        self.assertEqual(None, phone.GetDefaultEmail("Krzysztof Nowak"))

    def test_SetDefaultEmail_InContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("krzysztof.kowalczyk@gmail.com"), Email("krzysztof@gmail.com"), Email("krzysztof.kowalczyk@wp.pl"))

        self.assertEqual("krzysztof.kowalczyk@gmail.com", phone.GetDefaultEmail("Krzysztof Kowalczyk"))
        
        self.assertTrue(phone.setDefaultEmail("Krzysztof Kowalczyk", "krzysztof@gmail.com"))
        self.assertEqual("krzysztof@gmail.com", phone.GetDefaultEmail("Krzysztof Kowalczyk"))

    def test_SetDefaultEmail_InContactWrongEmail(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("krzysztof.kowalczyk@gmail.com"), Email("krzysztof@gmail.com"), Email("krzysztof.kowalczyk@wp.pl"))

        self.assertFalse(phone.setDefaultEmail("Krzysztof Kowalczyk", "krzysztof@test.com"))
        self.assertEqual("krzysztof.kowalczyk@gmail.com", phone.GetDefaultEmail("Krzysztof Kowalczyk"))

    def test_SetDefaultEmail_InContactWrongContact(self):
        phone = Contacts(self.__class__._fileName)
        phone.AddContact("Krzysztof","Kowalczyk", Email("krzysztof.kowalczyk@gmail.com"), Email("krzysztof@gmail.com"), Email("krzysztof.kowalczyk@wp.pl"))

        self.assertFalse(phone.setDefaultEmail("Krzysztof Nowak", "krzysztof@gmail.com"))
        self.assertEqual("krzysztof.kowalczyk@gmail.com", phone.GetDefaultEmail("Krzysztof Kowalczyk"))


if __name__=='__main__':
    unittest.main()

