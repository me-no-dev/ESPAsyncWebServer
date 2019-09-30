#!/bin/bash

BOARD=$1

echo -e "travis_fold:start:install_pio"
pip install -U platformio
if [ $? -ne 0 ]; then exit 1; fi

python -m platformio lib --storage-dir $TRAVIS_BUILD_DIR install
if [ $? -ne 0 ]; then exit 1; fi

python -m platformio lib -g install https://github.com/bblanchon/ArduinoJson.git
if [ $? -ne 0 ]; then exit 1; fi

case $BOARD in
esp32dev)
  python -m platformio lib -g install https://github.com/me-no-dev/AsyncTCP.git
  ;;
esp12e)
  python -m platformio lib -g install https://github.com/me-no-dev/ESPAsyncTCP.git || true
  ;;
esac
if [ $? -ne 0 ]; then exit 1; fi
echo -e "travis_fold:end:install_pio"

echo -e "travis_fold:start:test_pio"
for EXAMPLE in $TRAVIS_BUILD_DIR/examples/*/*.ino; do
    python -m platformio ci $EXAMPLE -l '.' -b $BOARD
    if [ $? -ne 0 ]; then exit 1; fi
done
echo -e "travis_fold:end:test_pio"
