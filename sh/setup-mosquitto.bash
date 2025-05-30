#!/usr/bin/env bash

sudo mkdir conf.d && cd conf.d/
sudo vim mqtt.conf

sudo mkdir ../certs && cd ../certs/
openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key -out server.crt -days 365 -subj "/CN=localhost"
sudo openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key -out server.crt -days 365 -subj "/CN=localhost"

sudo cp server.crt ca.crt

sudo chown root:mosquitto /etc/mosquitto/certs/server.key
sudo chown root:mosquitto /etc/mosquitto/certs/server.crt
sudo chown root:mosquitto /etc/mosquitto/certs/ca.crt

sudo chmod 640 /etc/mosquitto/certs/server.key
sudo chmod 644 /etc/mosquitto/certs/*.crt

sudo systemctl enable mosquitto
sudo systemctl restart mosquitto.service

sudo ufw allow 1883/tcp
sudo ufw allow 9001/tcp

ss -tulnp | grep mosquitto

# check mosquitto config
mosquitto -c /etc/mosquitto/mosquitto.conf
