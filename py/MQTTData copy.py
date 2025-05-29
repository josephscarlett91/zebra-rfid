import paho.mqtt.client as mqtt
import json
import pandas as pd
import csv
import os
import requests
import keyboard
import time
import uuid

from ReaderData import *

columns = ['CRC', 'PC', 'antenna', 'channel', 'eventNum', 'format', 'idHex', 'peakRssi', 
                    'phase', 'reads', 'timestamp', 'type']


command_id = str(uuid.uuid4())

payload = {
    "command": "start",
    "command_id": command_id,
    "payload": {}
}
payload_json = json.dumps(payload)


def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.publish("GTI/Zebra/Testing", payload_json)
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



mqttc.connect("broker.emqx.io", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
mqttc.loop_start()

while True:
    if keyboard.is_pressed('q'):
        print("quitting")
        break
    time.sleep(0.1)

    if keyboard.is_pressed('1'):
        start_inventory()
    if keyboard.is_pressed('2'):
        stop_inventory()

    if keyboard.is_pressed('m'):
        get_mode()
    if keyboard.is_pressed('n'):
        set_mode()

mqttc.loop_stop()
mqttc.disconnect()
