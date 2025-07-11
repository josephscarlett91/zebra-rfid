import paho.mqtt.client as mqtt
import json
import pandas as pd
import csv
import os
import requests
import keyboard
import time
import threading

from ReaderData import *

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("GTI/Zebra/Testing")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    #print(msg.topic+" "+str(msg.payload))
    # os.system('cls')

    decoded = msg.payload.decode('utf-8')
    data_dict = json.loads(decoded)
    
    if data_dict.get("type") == "heartbeat":
        return

    
    df = pd.DataFrame(data_dict)
    if 'data' in df:

        row = data_dict['data'].copy()
        row['timestamp'] = data_dict['timestamp']
        row['type'] = data_dict['type']

        write_row_to_csv(row)
        print(row)


def poll_reader_status():
    last_status = ""
    while True:
        try:
            status = get_status()
            radio_status = status.get("radioActivity", "unknown")

            if radio_status != last_status:
                print(f"[STATUS] Reader is now: {radio_status.upper()}")
                last_status = radio_status

            time.sleep(1) 
        except Exception as e:
            print(f"[ERROR] Failed to poll reader status: {e}")
            time.sleep(5)
            
def write_row_to_csv(row, file_path='tag_data.csv'):
    file_exists = os.path.isfile(file_path)
    write_header = not file_exists or os.stat(file_path).st_size == 0

    with open(file_path, 'a', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=row.keys())
        if write_header:
            writer.writeheader()
        writer.writerow(row)
        print(row)



mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message

mqttc.connect("192.168.12.19", 1883, 60)

mqttc.loop_start()

status_thread = threading.Thread(target=poll_reader_status, daemon=True)
status_thread.start()


while True:
    cmd = input("enter cmd: ").strip()
    if cmd == 'q':
        print("quitting")
        break

    if cmd == '1':
        print('starting...')
        start_inventory()
    if cmd == '2':
        print('stopping inventory...')
        stop_inventory()
        print('inventory stopped.')

    if cmd == 'm':
        print('loading mode details...')
        print(get_mode())

    if cmd == 't':
        type = input("input type: ")
        set_type(type)

    if cmd == 'p':
        power = int(input("input power: "))
        set_power(power)

    if cmd == 'a':
        antennas = input("input antennas in a comma separated list (eg. 1, 3, 4): ")
        antennas_arr = list(map(int, antennas.split(', ')))
        set_antennas(antennas)


    if cmd == 'w':
        print('connecting...')
        connect_wifi()
        
    # does not work
    if cmd == 'write':
        target_epc = input("Enter target EPC (hex string): ").strip()
        membank = input("Enter memory bank to write to (EPC, USER, TID, RESERVED): ").strip().upper()
        data = input("Enter hex data to write (must be multiple of 4 hex chars): ").strip()

        payload = {
            "type": "WRITE",
            "config": {
                "membank": membank,
                "wordPointer": 2,   # usually 2 to skip CRC/PC for EPC; you can adjust if needed
                "data": data,
                "blockSize": 0
            },
            "idHex": target_epc
        }

        mqttc.publish("GTI/Zebra/Testing", json.dumps(payload))
        print(f"Write command sent to tag {target_epc} on memory bank {membank}")

    time.sleep(0.1)

mqttc.loop_stop()
mqttc.disconnect()

