import paho.mqtt.client as mqtt
import json
import pandas as pd
import csv
import os
import requests
import time

from ReaderData import *

#columns = ['CRC', 'PC', 'antenna', 'channel', 'eventNum', 'format', 'idHex', 'peakRssi', 
#                    'phase', 'reads', 'timestamp', 'type']

columns = ['antenna', 'channel', 'eventNum', 'idHex', 'peakRssi', 'phase', 'reads', 'timestamp']

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
        row[type] = data_dict['type']

        file_path = 'tag_data.csv'
        with open(file_path, 'a', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=row.keys())

            if not os.path.isfile(file_path) or os.stat(file_path).st_size == 0:
                writer.writeheader()

            writer.writerow(row)
            print(row)




mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message



#mqttc.connect("broker.emqx.io", 1883, 60)
#mqttc.connect("192.168.12.19", 9001, 60)
#mqttc.connect("192.168.12.19", 1883, 60)
mqttc.connect("192.168.12.163", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
mqttc.loop_start()

print("Commands:")
print("[1] Start Inventory")
print("[2] Stop Inventory")
print("[m] Print Mode")
print("[n] Set Mode")
print("[t] Change Type")
print("[q] Quit")

while True:
    cmd = input("enter cmd ").strip()
    if cmd == 'q': 
        print("quitting")
        break
    
    if cmd == '1': 
        print('starting..')
        start_inventory()
    if cmd == '2': 
        print('stopping..')
        stop_inventory()

    if cmd == 'm': 
        print(get_mode())
    if cmd == 'n': 
        set_mode()

    if cmd == 't': 
        type = input("input type: ")
        change_type(type)

    if cmd == 'p': 
        power = int(input("input power: "))
        change_power(power)

    if cmd == 'w': 
        connect_wifi()

    time.sleep(0.1)

mqttc.loop_stop()
mqttc.disconnect()

