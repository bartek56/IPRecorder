import libGSMEngine as lib
import signal


class Killer:
    kill_now = False

    def __init__(self):
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGINT, self.exit_gracefully)

    def exit_gracefully(self, *args):
        self.kill_now = True


def main():
    print("start")
    killer = Killer()
    gsm = lib.GSMManager("/dev/ttyAMA0")
    gsm.initialize()
    gsm.sendSms("+48791942336", "hello world")
    while not killer.kill_now:
        if gsm.isNewSms():
            sms = gsm.getSms()
            print("new sms")
            print(sms.dateAndTime, sms.number)
            print(sms.msg)
            gsm.sendSms(sms.number, "thanks for message")


if __name__ == "__main__":
    main()
