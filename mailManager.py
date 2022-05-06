import smtplib 
from email.message import EmailMessage

class Mail():
    def __init__(self):
        file = open("mailAccount")
        lines = file.readlines()
        self.mail = lines[0].strip()
        self.password = lines[1].strip()

    def sendMail(self, address, text):
        msg = EmailMessage()
        msg.set_content(text)
        msg['Subject'] = "IPRecorder"
        msg['From'] = "iprecorder.server@gmail.com"
        msg['To'] = address

        server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        server.login(self.mail, self.password) 
        server.send_message(msg)

if __name__ == "__main__":
    mail = Mail()
    mail.sendMail("bartosz.brzozowski23@gmail.com", "everything perfectly work ")

