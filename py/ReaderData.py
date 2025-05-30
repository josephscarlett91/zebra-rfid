import paho.mqtt.client as mqtt
import json
import pandas as pd
import csv
import os
import requests
import copy

from schemas import *

requests.packages.urllib3.disable_warnings()


HEADERS = {
    "accept":"application/json",
    "Authorization": "Basic YWRtaW46R3RpMjAyNUA="
}

global my_mode
my_mode = copy.copy(MODE_TEMPLATE)

def get_auth_header():
    x = requests.get('https://169.254.231.229/cloud/localRestLogin', headers = HEADERS, verify = False)
    data = x.json()
    token = data.get("message")
    
    return {
    "accept":"application/json",
    "Authorization": f"Bearer {token}"
    }

def get_adjusted_header():
    x = get_auth_header()
    x["Content-Type"] = "application/json"
    return x


def start_inventory():
    requests.put('https://169.254.231.229/cloud/start', headers = get_auth_header(), verify = False)
    

def stop_inventory():
    requests.put('https://169.254.231.229/cloud/stop', headers = get_auth_header(), verify = False)
   
def get_mode():
    x = requests.get('https://169.254.231.229/cloud/mode', headers = get_auth_header(), verify = False)
    return (x.json())

def set_mode(mode_config):
    requests.put('https://169.254.231.229/cloud/mode', headers = get_adjusted_header(), json = mode_config, verify = False)


def set_type(type):
    old_type = get_mode()['type']
    my_mode['type'] = type
    set_mode(my_mode)
    if (old_type != get_mode()['type']):
        print(f"Successfuly set type to {type}")
    else:
        print(f"Could not set type to {type}, invalid input. Try: ")
        print("Simple \nInventory \nPortal \nConveyor \nCustom")

def set_power(power):
    my_mode['transmitPower'] = power
    set_mode(my_mode)
    if (0 <= power <= 33):
        print(f"Successfuly set power to {power}")
    else:
        print(f"Could not set power to {type}, invalid input. Input a number between 0 and 33.")

def set_antennas(antennas):
    my_mode['antennas'] = antennas
    set_mode(my_mode)

def get_status():
    x = requests.get('https://169.254.231.229/cloud/status', headers = get_auth_header(), verify = False)
    return (x.json())

def connect_wifi():
    network_config = {
    "mlan0": {
    "accesspoint": {
      "autoConn": True,
      "connect": True,
      "essid": "GTI",
      "security": {
        "enable": True,
        "WPA2Personal": {
          "password": "RADGroup2024"
        }
      }
    },
    "enable": True
  }
}
    requests.put('https://169.254.231.229/cloud/network', headers = get_adjusted_header(), json = network_config, verify = False)
