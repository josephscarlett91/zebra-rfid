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

columns = ['CRC', 'PC', 'antenna', 'channel', 'eventNum', 'format', 'idHex', 'peakRssi', 
                    'phase', 'reads', 'timestamp', 'type']


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



mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message



mqttc.connect("broker.emqx.io", 1883, 60)




# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.

mqttc.loop_start()

status_thread = threading.Thread(target=poll_reader_status, daemon=True)
status_thread.start()

while True:
    if keyboard.is_pressed('q'):
        print("quitting")
        break
    

    if keyboard.is_pressed('1'):
        print('starting..')
        start_inventory()
    if keyboard.is_pressed('2'):
        print('stopping..')
        stop_inventory()

    if keyboard.is_pressed('m'):
        print(get_mode())
    if keyboard.is_pressed('n'):
        set_mode()

    if keyboard.is_pressed('t'):
        type = input("input type: ")
        change_type(type)

    if keyboard.is_pressed('p'):
        power = int(input("input power: "))
        change_power(power)

    if keyboard.is_pressed('w'):
        connect_wifi()

    time.sleep(0.1)

mqttc.loop_stop()
mqttc.disconnect()

