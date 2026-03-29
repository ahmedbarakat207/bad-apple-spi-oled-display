#!/bin/bash

cd /Users/ahmed/bad_apple/platformio

# Upload sketch
pio run --target upload --upload-port /dev/cu.usbserial-0001

# Upload LittleFS data folder
pio run --target uploadfs --upload-port /dev/cu.usbserial-0001
