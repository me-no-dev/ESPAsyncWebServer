#!/bin/bash

BOARD=$1

echo -e "travis_fold:start:install_pio"
pip install -U platformio

python -m platformio lib install https://github.com/bblanchon/ArduinoJson.git

case $BOARD in
esp32dev)
  python -m platformio lib -g install https://github.com/me-no-dev/AsyncTCP.git
  ;;
esp12e)
  python -m platformio lib -g install https://github.com/me-no-dev/ESPAsyncTCP.git || true
  ;;
esac
echo -e "travis_fold:end:install_pio"

echo -e "travis_fold:start:test_pio"
for EXAMPLE in $PWD/examples/*/*.ino; do
    python -m platformio ci $EXAMPLE -l '.' -b $BOARD
done
echo -e "travis_fold:end:test_pio"
