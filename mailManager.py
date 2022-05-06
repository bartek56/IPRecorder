import os
import smtplib 

from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart

class Mail():
    def __init__(self, mailAccountFile):
        file = open(mailAccountFile)
        lines = file.readlines()
        self.mail = lines[0].strip()
        self.password = lines[1].strip()

    def sendMail(self, address, text, attachments=[]):
        msg = MIMEMultipart()

        msg['Subject'] = "IPRecorder"
        msg['From'] = self.mail
        msg['To'] = address
        msg.attach(MIMEText(text))

        for x in attachments:
            f = open(x,'rb')
            img_data = f.read()
            f.close()
            image = MIMEImage(img_data, name=os.path.basename(x))
            msg.attach(image)

        server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        server.login(self.mail, self.password) 
        server.send_message(msg)

if __name__ == "__main__":
    mail = Mail("mailAccount")
    images = ["/home/bartosz/Pictures/tapety/20160402_095632.jpg","/home/bartosz/Pictures/tapety/20160806_124619.jpg", "/home/bartosz/Pictures/tapety/20170507_203252.jpg"]
    mail.sendMail("bartosz.brzozowski23@gmail.com", "everything perfectly work", images)

