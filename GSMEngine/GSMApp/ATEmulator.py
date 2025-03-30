import serial
import threading
import keyboard
import argparse
import time


def read_serial(ser):
    """Odczytuje dane z portu szeregowego i wyświetla je na ekranie."""
    while True:
        try:
            data = ser.readline().decode('utf-8').strip()
            if data:
                print(f"\n[Odebrane] {data}")
        except Exception as e:
            print(f"Błąd odczytu: {e}")
            break


def send_command(ser, key):
    """Wysyła polecenie przez port szeregowy na podstawie naciśniętego klawisza."""
    command = f"{key}"
    ser.write(command.encode('utf-8'))
    print(f"[Wysłano] {command.strip()}")


def main():
    parser = argparse.ArgumentParser(description="Program do komunikacji przez port szeregowy.")
    parser.add_argument("port", type=str, help="Nazwa portu szeregowego (np. COM3 lub /dev/ttyUSB0)")
    args = parser.parse_args()
    baudrate = 19200

    try:
        ser = serial.Serial(args.port, baudrate, timeout=1)
        print("\nPołączono z portem szeregowym.")
        print("Dostępne polecenia:\n", "0 - ATE0\n", "1 - OK\n",
              "2 - send '>' for sms\n", "3 - +CMGS for sms")

        # Uruchamiamy wątek do odbioru danych
        threading.Thread(target=read_serial, args=(ser,), daemon=True).start()

        # Nasłuchujemy klawiszy 0-9
        while True:
            event = keyboard.read_event()
            if event.event_type == keyboard.KEY_DOWN and event.name in "0123456789":
                if event.name == "0":
                    send_command(ser, "ATE0\r\n")
                    time.sleep(0.5)
                    send_command(ser, "OK\r\n")
                    time.sleep(0.2)
                if event.name == "1":
                    send_command(ser, "OK\r\n")
                    time.sleep(0.2)
                if event.name == "2":
                    send_command(ser, ">\r\n")
                    time.sleep(2)
                if event.name == "3":
                    send_command(ser, "+CMGS\r\n")
                    time.sleep(0.2)
                    send_command(ser, "OK\r\n")
                    time.sleep(0.2)

    except serial.SerialException as e:
        print(f"Błąd otwarcia portu: {e}")
    except KeyboardInterrupt:
        print("\nZamykanie programu...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Port zamknięty.")


if __name__ == "__main__":
    main()
