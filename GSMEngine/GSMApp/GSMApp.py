import GSMEngine as lib
import signal
from time import sleep

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
    gsm = lib.GSMManager("/dev/pts/3")
    gsm.initialize()
    gsm.sendSms("+48791942336", "hello world")
    print("after sync message")
    #gsm.sendSmsSync("+48791942336", "hello world 2")
    #print("after async message")
    while not killer.kill_now:
        if gsm.isNewSms():
            sms = gsm.getSms()
            print("new sms")
            print(sms.dateAndTime, sms.number)
            print(sms.msg)
            gsm.sendSms(sms.number, "thanks for message")
        sleep(0.2)


if __name__ == "__main__":
    main()
