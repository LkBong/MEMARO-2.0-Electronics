import argparse
import serial
import csv
import datetime
import time
import os

COM_PORT = "COM4"  # change to match your system (check Arduino IDE → Tools → Port)

parser = argparse.ArgumentParser()
parser.add_argument("name", nargs="?", help="output filename stem (saved as data/<name>.csv)")
args = parser.parse_args()

if args.name:
    filename = f'./data/{args.name}.csv'
else:
    date_time = datetime.datetime.now().strftime("%m-%d_%H-%M")
    filename = f'./data/log_{date_time}.csv'

os.makedirs('./data', exist_ok=True)
with open(filename, mode='w', newline='') as logfile:
    writer = csv.writer(logfile)
    writer.writerow(["pc_time", "elapsed_ms", "linear-speed_ms-1", "current_a"])

    ser = serial.Serial(COM_PORT, 115200)
    time.sleep(2)       # wait for ESP32 to finish booting after DTR reset
    ser.flushInput()    # discard all boot messages

    ser.write(b"ping\n")
    ack = ser.readline().decode("utf-8").strip()
    if ack != "pong":
        print(f"Unexpected handshake response: {ack!r}")
    else:
        print("ESP32 ready — press GPIO0 button to start test")

    while True:
        decoded = ser.readline().decode("utf-8").strip()

        if decoded == "stop":
            break
        if decoded == "start":
            continue  # skip sync marker, not a data row

        pc_time = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
        writer.writerow([pc_time] + decoded.split(','))

    # Keep port open until return phase finishes.
    # Closing it early toggles DTR/RTS and resets the ESP32 mid-reverse.
    print("Logging done — waiting for motor return phase...")
    while True:
        line = ser.readline().decode("utf-8").strip()
        if line == "reset_complete":
            break

    ser.close()

print("logging finished")
