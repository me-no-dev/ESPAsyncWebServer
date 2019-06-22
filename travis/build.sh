#!/bin/bash

echo -e "travis_fold:start:sketch_test_env_prepare"
pip install pyserial
wget -O arduino.tar.xz https://www.arduino.cc/download.php?f=/arduino-nightly-linux64.tar.xz
tar xf arduino.tar.xz
mv arduino-nightly $HOME/arduino_ide
mkdir -p $HOME/Arduino/libraries
cd $HOME/Arduino/libraries
cp -rf $TRAVIS_BUILD_DIR ESPAsyncWebServer
git clone https://github.com/bblanchon/ArduinoJson
git clone https://github.com/me-no-dev/AsyncTCP
cd $HOME/arduino_ide/hardware
mkdir espressif
cd espressif
git clone https://github.com/espressif/arduino-esp32.git esp32
cd esp32
git submodule update --init --recursive
cd tools
python get.py
cd $TRAVIS_BUILD_DIR
export PATH="$HOME/arduino_ide:$HOME/arduino_ide/hardware/espressif/esp32/tools/xtensa-esp32-elf/bin:$PATH"
source travis/common.sh
echo -e "travis_fold:end:sketch_test_env_prepare"

echo -e "travis_fold:start:sketch_test"
build_sketches $HOME/arduino_ide $HOME/Arduino/libraries/ESPAsyncWebServer/examples "-l $HOME/Arduino/libraries"
if [ $? -ne 0 ]; then exit 1; fi
echo -e "travis_fold:end:sketch_test"
