#!/usr/bin/env python3

import os
import sys
import time
import select
import termios
import tty
import threading
from datetime import datetime


SERIAL = "/dev/pts/5"


class ATModem:

    def __init__(self, serial):
        self.serial = serial

        self.running = True

        # Synchronizacja dostępu do PTY
        self.write_lock = threading.Lock()

        # -----------------------------------------------------
        # Modem state
        # -----------------------------------------------------

        self.sim_ready = True
        self.registered = True
        self.attached = True

        self.signal_quality = 20
        self.operator = "TestOperator"

        # 0 = PDU
        # 1 = Text mode
        self.sms_mode = 1

        self.echo = True

        # -----------------------------------------------------
        # SMS
        # -----------------------------------------------------

        self.sms_storage = []

        self.waiting_for_sms = False

        # -----------------------------------------------------
        # Serial
        # -----------------------------------------------------

        self.fd = None
        self.old_termios = None

    # =========================================================
    # SERIAL
    # =========================================================

    def open(self):

        self.fd = os.open(
            self.serial,
            os.O_RDWR | os.O_NOCTTY
        )

        self.old_termios = termios.tcgetattr(
            self.fd
        )

        tty.setraw(self.fd)

        print(
            f"[MODEM] Opened {self.serial}"
        )

    def close(self):

        if self.fd is not None:

            if self.old_termios is not None:

                termios.tcsetattr(
                    self.fd,
                    termios.TCSANOW,
                    self.old_termios
                )

            os.close(self.fd)

            self.fd = None

    # =========================================================
    # SERIAL OUTPUT
    # =========================================================

    def write(self, data):

        if isinstance(data, str):
            data = data.encode()

        with self.write_lock:

            os.write(
                self.fd,
                data
            )

        print(
            f"[MODEM -> HOST RAW] {data!r}"
        )

    def response(self, text):

        self.write(
            text + "\r\n"
        )

    # =========================================================
    # AT COMMANDS
    # =========================================================

    def handle_command(self, command):

        command = command.strip()

        if not command:
            return

        print(
            f"[HOST -> MODEM] {command!r}"
        )

        # -----------------------------------------------------
        # Echo
        # -----------------------------------------------------

        if self.echo:

            self.response(
                command
            )

        # -----------------------------------------------------
        # AT
        # -----------------------------------------------------

        if command == "AT":

            self.response("OK")
            return

        # -----------------------------------------------------
        # Echo OFF
        # -----------------------------------------------------

        if command == "ATE0":

            self.echo = False

            self.response("OK")

            return

        # -----------------------------------------------------
        # Echo ON
        # -----------------------------------------------------

        if command == "ATE1":

            self.echo = True

            self.response("OK")

            return

        # -----------------------------------------------------
        # SIM
        # -----------------------------------------------------

        if command == "AT+CPIN?":

            if self.sim_ready:

                self.response(
                    "+CPIN: READY"
                )

            else:

                self.response(
                    "+CPIN: SIM PIN"
                )

            self.response("OK")

            return

        # -----------------------------------------------------
        # Signal
        # -----------------------------------------------------

        if command == "AT+CSQ":

            self.response(
                f"+CSQ: "
                f"{self.signal_quality},99"
            )

            self.response("OK")

            return

        # -----------------------------------------------------
        # Registration
        # -----------------------------------------------------

        if command == "AT+CREG?":

            if self.registered:

                self.response(
                    "+CREG: 0,1"
                )

            else:

                self.response(
                    "+CREG: 0,0"
                )

            self.response("OK")

            return

        # -----------------------------------------------------
        # GPRS attach
        # -----------------------------------------------------

        if command == "AT+CGATT?":

            if self.attached:

                self.response(
                    "+CGATT: 1"
                )

            else:

                self.response(
                    "+CGATT: 0"
                )

            self.response("OK")

            return

        # -----------------------------------------------------
        # Operator
        # -----------------------------------------------------

        if command == "AT+COPS?":

            self.response(
                f'+COPS: 0,0,'
                f'"{self.operator}",7'
            )

            self.response("OK")

            return

        # -----------------------------------------------------
        # SMS mode
        # -----------------------------------------------------

        if command == "AT+CMGF?":

            self.response(
                f"+CMGF: {self.sms_mode}"
            )

            self.response("OK")

            return

        if command.startswith("AT+CMGF="):

            try:

                mode = int(
                    command.split("=", 1)[1]
                )

                if mode not in (0, 1):

                    self.response("ERROR")

                    return

                self.sms_mode = mode

                self.response("OK")

            except ValueError:

                self.response("ERROR")

            return

        # -----------------------------------------------------
        # Send SMS
        # -----------------------------------------------------

        if command.startswith("AT+CMGS="):

            self.start_sms_send(
                command
            )

            return

        # -----------------------------------------------------
        # List SMS
        # -----------------------------------------------------

        if command == 'AT+CMGL="ALL"':

            self.list_sms()

            return

        # -----------------------------------------------------
        # Read SMS
        # -----------------------------------------------------

        if command.startswith("AT+CMGR="):

            self.read_sms(
                command
            )

            return

        # -----------------------------------------------------
        # Delete SMS
        # -----------------------------------------------------

        if command.startswith("AT+CMGD="):

            self.delete_sms(
                command
            )

            return

        # -----------------------------------------------------
        # Unknown command
        # -----------------------------------------------------

        print(
            f"[MODEM] Unknown command: "
            f"{command}"
        )

        self.response("ERROR")

    # =========================================================
    # OUTGOING SMS
    # =========================================================

    def start_sms_send(self, command):

        print(
            f"[MODEM] SMS send requested: "
            f"{command}"
        )

        self.waiting_for_sms = True

        # Modem prompt
        self.write("> ")

    def process_sms(self, text):

        print(
            f"[MODEM] SMS content: "
            f"{text!r}"
        )

        self.waiting_for_sms = False

        text = text.replace(
            "\x1a",
            ""
        )

        time.sleep(0.5)

        self.response(
            "+CMGS: 1"
        )

        self.response(
            "OK"
        )

    # =========================================================
    # SMS STORAGE
    # =========================================================

    def list_sms(self):

        for index, sms in enumerate(
            self.sms_storage
        ):

            self.response(
                f'+CMGL: {index},1,'
                f'"REC READ",'
                f'"{sms["number"]}",,'
                f'"{sms["date"]}"'
            )

            self.response(
                sms["message"]
            )

        self.response("OK")

    def read_sms(self, command):

        try:

            index = int(
                command.split("=", 1)[1]
            )

            sms = self.sms_storage[index]

            self.response(
                f'+CMGR: 1,"REC READ",'
                f'"{sms["number"]}",,'
                f'"{sms["date"]}"'
            )

            self.response(
                sms["message"]
            )

            self.response("OK")

        except (
            ValueError,
            IndexError
        ):

            self.response("ERROR")

    def delete_sms(self, command):

        try:

            index = int(
                command.split("=", 1)[1]
            )

            del self.sms_storage[index]

            self.response("OK")

        except (
            ValueError,
            IndexError
        ):

            self.response("ERROR")

    # =========================================================
    # INCOMING SMS
    # =========================================================

    def receive_sms(
        self,
        number,
        message
    ):

        now = datetime.now()

        date = now.strftime(
            "%y/%m/%d,%H:%M:%S+08"
        )

        print()
        print(
            "[MODEM] <<< INCOMING SMS >>>"
        )

        print(
            f"[MODEM] From: {number}"
        )

        print(
            f"[MODEM] Message: {message}"
        )

        # -----------------------------------------------------
        # Save SMS
        # -----------------------------------------------------

        sms = {
            "number": number,
            "message": message,
            "date": date,
        }

        self.sms_storage.append(
            sms
        )

        # -----------------------------------------------------
        # Send unsolicited message
        # -----------------------------------------------------

        self.write(
            f'+CMT: "{number}",,"{date}"'
            f'\r\n'
        )

        self.write(
            f"{message}\r\n"
        )

        print(
            "[MODEM] <<< SMS SENT TO HOST >>>"
        )

    # =========================================================
    # MODEM STATUS
    # =========================================================

    def print_status(self):

        print()
        print("========== MODEM STATUS ==========")

        print(
            f"Serial:       {self.serial}"
        )

        print(
            f"SIM:          "
            f"{'READY' if self.sim_ready else 'NOT READY'}"
        )

        print(
            f"Network:      "
            f"{'REGISTERED' if self.registered else 'NOT REGISTERED'}"
        )

        print(
            f"GPRS:         "
            f"{'ATTACHED' if self.attached else 'DETACHED'}"
        )

        print(
            f"Signal:       {self.signal_quality}"
        )

        print(
            f"Operator:     {self.operator}"
        )

        print(
            f"SMS mode:     {self.sms_mode}"
        )

        print(
            f"Echo:         "
            f"{'ON' if self.echo else 'OFF'}"
        )

        print(
            f"Stored SMS:   {len(self.sms_storage)}"
        )

        print(
            "=================================="
        )

        print()

    # =========================================================
    # CONSOLE COMMANDS
    # =========================================================

    def console_help(self):

        print()
        print("Available commands:")
        print()
        print(
            "  sms <number> <message>"
        )
        print(
            "      Simulate incoming SMS"
        )
        print()
        print(
            "  signal <0-31>"
        )
        print(
            "      Change signal quality"
        )
        print()
        print(
            "  network on"
        )
        print(
            "  network off"
        )
        print(
            "      Enable/disable network"
        )
        print()
        print(
            "  gprs on"
        )
        print(
            "  gprs off"
        )
        print(
            "      Attach/detach GPRS"
        )
        print()
        print(
            "  sim ready"
        )
        print(
            "  sim locked"
        )
        print(
            "      Change SIM state"
        )
        print()
        print(
            "  status"
        )
        print(
            "      Show modem state"
        )
        print()
        print(
            "  sms-list"
        )
        print(
            "      Show stored SMS"
        )
        print()
        print(
            "  help"
        )
        print(
            "      Show this help"
        )
        print()
        print(
            "  quit"
        )
        print(
            "      Exit emulator"
        )
        print()

    def console_loop(self):

        print()
        print(
            "======================================"
        )
        print(
            "         AT MODEM EMULATOR"
        )
        print(
            "======================================"
        )

        print(
            f"Serial: {self.serial}"
        )

        print(
            "Type 'help' for available commands."
        )

        print()

        while self.running:

            try:

                command = input(
                    "MODEM> "
                ).strip()

            except EOFError:

                break

            except KeyboardInterrupt:

                print()

                self.running = False

                break

            if not command:
                continue

            # -------------------------------------------------
            # HELP
            # -------------------------------------------------

            if command == "help":

                self.console_help()

                continue

            # -------------------------------------------------
            # STATUS
            # -------------------------------------------------

            if command == "status":

                self.print_status()

                continue

            # -------------------------------------------------
            # INCOMING SMS
            #
            # sms +48123456789 hello world
            # -------------------------------------------------

            if command.startswith("sms "):

                parts = command.split(
                    " ",
                    2
                )

                if len(parts) < 3:

                    print(
                        "Usage:"
                    )

                    print(
                        "  sms <number> <message>"
                    )

                    continue

                number = parts[1]
                message = parts[2]

                self.receive_sms(
                    number,
                    message
                )

                continue

            # -------------------------------------------------
            # SIGNAL
            # -------------------------------------------------

            if command.startswith("signal "):

                try:

                    value = int(
                        command.split(
                            " ",
                            1
                        )[1]
                    )

                    if value < 0 or value > 31:

                        raise ValueError

                    self.signal_quality = value

                    print(
                        f"[CONSOLE] Signal = "
                        f"{value}"
                    )

                except ValueError:

                    print(
                        "Signal must be "
                        "between 0 and 31."
                    )

                continue

            # -------------------------------------------------
            # NETWORK
            # -------------------------------------------------

            if command == "network on":

                self.registered = True

                print(
                    "[CONSOLE] Network ON"
                )

                continue

            if command == "network off":

                self.registered = False

                print(
                    "[CONSOLE] Network OFF"
                )

                continue

            # -------------------------------------------------
            # GPRS
            # -------------------------------------------------

            if command == "gprs on":

                self.attached = True

                print(
                    "[CONSOLE] GPRS ATTACHED"
                )

                continue

            if command == "gprs off":

                self.attached = False

                print(
                    "[CONSOLE] GPRS DETACHED"
                )

                continue

            # -------------------------------------------------
            # SIM
            # -------------------------------------------------

            if command == "sim ready":

                self.sim_ready = True

                print(
                    "[CONSOLE] SIM READY"
                )

                continue

            if command == "sim locked":

                self.sim_ready = False

                print(
                    "[CONSOLE] SIM LOCKED"
                )

                continue

            # -------------------------------------------------
            # SMS LIST
            # -------------------------------------------------

            if command == "sms-list":

                if not self.sms_storage:

                    print(
                        "[CONSOLE] No stored SMS."
                    )

                else:

                    for index, sms in enumerate(
                        self.sms_storage
                    ):

                        print(
                            f"[{index}] "
                            f"{sms['number']}: "
                            f"{sms['message']}"
                        )

                continue

            # -------------------------------------------------
            # QUIT
            # -------------------------------------------------

            if command in (
                "quit",
                "exit"
            ):

                print(
                    "[CONSOLE] Stopping emulator..."
                )

                self.running = False

                break

            # -------------------------------------------------
            # UNKNOWN
            # -------------------------------------------------

            print(
                f"Unknown command: {command}"
            )

            print(
                "Type 'help' for available commands."
            )

    # =========================================================
    # SERIAL LOOP
    # =========================================================

    def serial_loop(self):

        buffer = b""

        while self.running:

            try:

                readable, _, _ = select.select(
                    [self.fd],
                    [],
                    [],
                    0.5
                )

                if not readable:
                    continue

                data = os.read(
                    self.fd,
                    1024
                )

                if not data:
                    continue

                print(
                    f"[HOST -> MODEM RAW] "
                    f"{data!r}"
                )

                buffer += data

                # -------------------------------------------------
                # SMS body
                # -------------------------------------------------

                if self.waiting_for_sms:

                    if b"\x1a" in buffer:

                        sms_data, buffer = buffer.split(
                            b"\x1a",
                            1
                        )

                        text = sms_data.decode(
                            errors="replace"
                        )

                        self.process_sms(
                            text
                        )

                        continue

                # -------------------------------------------------
                # AT commands
                # -------------------------------------------------

                while b"\r" in buffer:

                    line, buffer = buffer.split(
                        b"\r",
                        1
                    )

                    # CRLF -> remove LF
                    if buffer.startswith(
                        b"\n"
                    ):

                        buffer = buffer[1:]

                    if not line:
                        continue

                    command = line.decode(
                        errors="replace"
                    )

                    self.handle_command(
                        command
                    )

            except OSError as e:

                print(
                    f"[MODEM] Serial error: {e}"
                )

                break

    # =========================================================
    # MAIN
    # =========================================================

    def run(self):

        self.open()

        # -----------------------------------------------------
        # Serial communication thread
        # -----------------------------------------------------

        serial_thread = threading.Thread(
            target=self.serial_loop,
            daemon=True
        )

        serial_thread.start()

        # -----------------------------------------------------
        # Console runs in main thread
        # -----------------------------------------------------

        try:

            self.console_loop()

        finally:

            self.running = False

            self.close()


# =============================================================
# MAIN
# =============================================================

def main():

    serial = SERIAL

    if len(sys.argv) > 1:

        serial = sys.argv[1]

    modem = ATModem(
        serial
    )

    modem.run()


if __name__ == "__main__":

    main()