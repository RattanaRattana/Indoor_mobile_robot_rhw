#!/bin/sh

# set up can communication for usb-can for laptop
# for asda b3 driver, bitrate is 500000
# for md 200T driver bitrate is 50000

sudo ip link set can0 up type can bitrate 500000 

sudo ifconfig can0 txqueuelen 65536
