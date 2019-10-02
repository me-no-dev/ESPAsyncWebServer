#!/bin/bash

echo "Installing ESP8266 Arduino Core in '$ARDUINO_USR_PATH/hardware/esp8266com'..."
script_init_path="$PWD"
mkdir -p "$ARDUINO_USR_PATH/hardware/esp8266com" && \
cd "$ARDUINO_USR_PATH/hardware/esp8266com" && \
echo "Cloning Core Repository..." && \
git clone https://github.com/esp8266/Arduino.git esp8266 > /dev/null 2>&1
if [ $? -ne 0 ]; then echo "ERROR: GIT clone failed"; exit 1; fi
cd esp8266 && \
echo "Updating submodules..." && \
git submodule update --init --recursive > /dev/null 2>&1
if [ $? -ne 0 ]; then echo "ERROR: Submodule update failed"; exit 1; fi
echo "Installing Python Serial..." && \
pip install pyserial > /dev/null
if [ $? -ne 0 ]; then echo "ERROR: Install failed"; exit 1; fi
cd tools
if [ "$OS_IS_WINDOWS" == "1" ]; then
	echo "Installing Python Requests..."
	pip install requests > /dev/null
	if [ $? -ne 0 ]; then echo "ERROR: Install failed"; exit 1; fi
fi
echo "Installing Platform Tools..."
python get.py > /dev/null
if [ $? -ne 0 ]; then echo "ERROR: Download failed"; exit 1; fi
cd $script_init_path

echo "ESP8266 Arduino has been installed in '$ARDUINO_USR_PATH/hardware/esp8266com'"
echo ""
