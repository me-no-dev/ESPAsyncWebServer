#!/bin/bash

if [ ! -z "$TRAVIS_BUILD_DIR" ]; then
	export GITHUB_WORKSPACE="$TRAVIS_BUILD_DIR"
	export GITHUB_REPOSITORY="$TRAVIS_REPO_SLUG"
elif [ -z "$GITHUB_WORKSPACE" ]; then
	export GITHUB_WORKSPACE="$PWD"
	export GITHUB_REPOSITORY="me-no-dev/ESPAsyncWebServer"
fi

TARGET_PLATFORM="$1"
CHUNK_INDEX=$2
CHUNKS_CNT=$3
BUILD_PIO=0
if [ "$#" -lt 1 ]; then
	TARGET_PLATFORM="esp32"
fi
if [ "$#" -lt 3 ] || [ "$CHUNKS_CNT" -le 0 ]; then
	CHUNK_INDEX=0
	CHUNKS_CNT=1
elif [ "$CHUNK_INDEX" -gt "$CHUNKS_CNT" ]; then
	CHUNK_INDEX=$CHUNKS_CNT
elif [ "$CHUNK_INDEX" -eq "$CHUNKS_CNT" ]; then
	BUILD_PIO=1
fi

if [ "$BUILD_PIO" -eq 0 ]; then
	# ArduinoIDE Test
	source ./.github/scripts/install-arduino-ide.sh

	if [ $PLATFORM == "esp32" ]; then
		FQBN="espressif:esp32:esp32:PSRAM=enabled,PartitionScheme=huge_app"
		source ./.github/scripts/install-arduino-core-esp32.sh
		echo "BUILDING ESP32 EXAMPLES"
	else
		FQBN="esp8266com:esp8266:generic:eesz=4M1M,ip=lm2f"
		source ./.github/scripts/install-arduino-core-esp8266.sh
		echo "BUILDING ESP8266 EXAMPLES"
	fi
	build_sketches "$FQBN" "$GITHUB_WORKSPACE/examples" "$CHUNK_INDEX" "$CHUNKS_CNT"
else
	# PlatformIO Test
	source ./.github/scripts/install-platformio-esp32.sh
	python -m platformio lib --storage-dir $GITHUB_WORKSPACE install
	python -m platformio lib -g install https://github.com/bblanchon/ArduinoJson.git
	if [ $PLATFORM == "esp32" ]; then
		BOARD="esp32dev"
		python -m platformio lib -g install https://github.com/me-no-dev/AsyncTCP.git
		echo "BUILDING ESP32 EXAMPLES"
	else
		BOARD="esp12e"
		python -m platformio lib -g install https://github.com/me-no-dev/ESPAsyncTCP.git
		echo "BUILDING ESP8266 EXAMPLES"
	fi
	build_pio_sketches $BOARD "$GITHUB_WORKSPACE/examples"
fi
if [ $? -ne 0 ]; then exit 1; fi
