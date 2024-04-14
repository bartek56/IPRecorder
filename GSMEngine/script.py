import libGSMEngine as lib

gsm = lib.GSMManager("/dev/pts/1")
gsm.initialize()
gsm.sendSMS("Hello World", 791942336)
