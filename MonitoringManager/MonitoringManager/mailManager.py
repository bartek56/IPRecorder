from simplegmail import Gmail

class Mail():
    def __init__(self):
        self.gmail = Gmail() # will open a browser window to ask you to log in and authenticate

    def sendMail(self, address, subject, text, attachments=[]):

        params = {
              "to": address,
              "sender": "iprecorder.server@gmail.com",
              "subject": subject,
              "msg_html": text,
              "signature": True  # use my account signature
        }
        message = self.gmail.send_message(**params)  # equivalent to send_message(to="you@youremail.com", sender=...)

if __name__ == "__main__":
    mail = Mail()
    mail.sendMail("bartek5_6@ineria.pl", "IPRecorder", "everything perfectly work")
