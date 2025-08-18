#!/bin/bash

# Setup verbose output and fail on errors
export PS4="\e[34m>> \e[37m"; set -x; set -e

# Cleanup
function cleanup_and_exit() {
	local exit_code=$?

	set +e;
	rm a.* 2>/dev/null

	if [ "$exit_code" -eq 0 ]; then
		echo -e "\e[32m\nSUCCESS!\e[0m"
	else
		echo -e "\e[31m\nERROR!\e[0m"
	fi
}

# Trap exit
trap cleanup_and_exit EXIT

###############################################################################
# CONFIGURATION:
###############################################################################
COMPILER="$PWD"/tools/arduino-cli

SERIAL_PORT=COM3
MONITOR_BAUD=921600
export OTA_IP="10.10.10.10"

export TARGET_NAME="psuBOX"
export TARGET=psu_box_esp32
#export TARGET=touch_calib

#EXTRA_FLAGS="-v"

###############################################################################
# TARGETS AVAILABLE
###############################################################################

if [ "$TARGET" == "psu_box_esp32" ] || [ "$TARGET" == "touch_calib" ]; then
	BOARD=esp32:esp32:nodemcu-32s
	UPLOADSPEED=:UploadSpeed=921600

	PROPS='--build-property build.partitions=partitions
	       --build-property upload.maximum_size=1638400'
	echo "PROPS: " ${PROPS}
else
	echo "Bad target!"
	exit 1
fi

###############################################################################
# MAIN
###############################################################################
compile() {
	# enumerate (and empty) all generated files
	mkdir -p build

	rm build/* 2> /dev/null || true # Clean build
	

	echo "Initial setup..."

	# Setup tools and libraries
	./setup.sh
	if [[ $? -ne 0 ]]; then
	    echo "FATAL ERROR: Setup failed."
	    exit 1
	fi


	echo "Testing..."

	# Setup tools and libraries
	cd ui
	./make.sh
	if [[ $? -ne 0 ]]; then
	    echo "FATAL ERROR: Test failed."
	    exit 1
	fi
	cd ..

	echo "Copying..."

	# Copy all necessary files into build/
	#cp *.ino build/build.ino
	if [ "$TARGET" == "touch_calib" ]; then
		cp touch_calib.ino build/build.ino
		cp tft_setup.h build/
	else
		cp psuBOX.ino build/build.ino
		cp *.h build/
		cp libraries/eltek_valere_psu/*.h build/
		cp ui/lv_font_montserrat_12_ukrainian.c build/
		cp ui/lv_font_montserrat_14_ukrainian.c build/
		cp ui/psu_box_ui.h build/
	fi
	cp *.csv build/


	echo "Compiling..."

	# Goto build directory with all generated files
	cd build

	while ! ${COMPILER} compile -b ${BOARD}${FQBN} --warnings "all" \
			   ${PROPS} -e --libraries "../libraries/" \
			   --library "../ui/build/_deps/lvgl-src" \
			   ${EXTRA_FLAGS}; do
		read -p "Press any key to continue "
		exit
	done
}

# Function to monitor
monitor() {
	if [ -n "${SERIAL_PORT+x}" ]; then
		while true; do
			${COMPILER} monitor -p ${SERIAL_PORT} \
			      --config baudrate=${MONITOR_BAUD} ${EXTRA_FLAGS};
			sleep 1
		done
	fi
}

upload() {
	if [ -n "${SERIAL_PORT+x}" ]; then
		while ! ${COMPILER} upload -b ${BOARD}${FQBN} \
				   -p ${SERIAL_PORT} ${EXTRA_FLAGS}; do
			sleep 1
		done
	fi
}

web_upload() {
	local BOARD_PATH="${BOARD//:/\.}"  # Replace ':' with '.'
	local FILE="build/${BOARD_PATH}/$(basename "$PWD").ino.bin"

	if [ ! -f "$FILE" ]; then
		echo "Firmware file not found: $FILE"
		return 1
	fi

	while true; do
		echo "Uploading $FILE to http://${OTA_IP}/update..."
		curl -F "file=@$FILE" http://${OTA_IP}/update && break
		echo "Upload failed, retrying in 1 second..."
		sleep 1
	done

	echo "Upload complete."
}

# Check if the argument is "monitor"
if [ "$1" == "monitor" ]; then
	monitor
elif [ "$1" == "upload" ]; then
	cd build
	upload
elif [ "$1" == "flash" ]; then
	cd build
	upload
elif [[ "$1" == "web" && ( "$2" == "upload" || "$2" == "flash" ) ]]; then
	cd build
	web_upload
elif [ "$1" == "web" ]; then
	compile
	web_upload
else
	compile
	upload
	monitor
fi

read -p "Press any key to continue "
