# OUTLINE

 - stream mqtt data
 - start stop on demo for reading
 - configure rfid reader configuration with http
 - setup local mqtt endpoint

# OVERVIEW


## /etc/
directory containing mosquitto configuration
mqtt configuration for different ports

## sh/
directory containing bash
for convenient http api calls 

## py/ 
directory containing python script for mqtt

## c/ 
code runs both 

 - api calls and 
 - python mqtt in one 
 - filters cloud mode

 gcc -o run both.c -ljansson -lcurl -lncurses -lpaho-mqtt3c -lpthread

 requires 
 
 - ncurses
 - jannson
 - paho-mqtt
 - curl


