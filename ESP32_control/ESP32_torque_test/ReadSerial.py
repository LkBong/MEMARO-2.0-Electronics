import serial
import csv
import datetime
import time
import os

COM_PORT = "COM4"  # change to match your system (check Arduino IDE → Tools → Port)

now = datetime.datetime.now()
date_time = now.strftime("%m-%d_%H-%M")

os.makedirs('./data', exist_ok=True)
with open(f'./data/log_{date_time}.csv', mode='w', newline='') as logfile:
    writer = csv.writer(logfile)
    writer.writerow(["pc_time", "elapsed_ms", "speed_rpm", "current_a"])

    ser = serial.Serial(COM_PORT, 115200)
    time.sleep(2)       # wait for ESP32 to finish booting after DTR reset
    ser.flushInput()    # discard all boot messages

    ser.write(b"ping\n")
    ack = ser.readline().decode("utf-8").strip()
    if ack != "pong":
        print(f"Unexpected handshake response: {ack!r}")
    else:
        print("ESP32 ready — press GPIO21 button to start test")

    while True:
        decoded = ser.readline().decode("utf-8").strip()

        if decoded == "stop":
            break
        if decoded == "start":
            continue  # skip sync marker, not a data row

        pc_time = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
        writer.writerow([pc_time] + decoded.split(','))

    ser.close()

print("logging finished")
