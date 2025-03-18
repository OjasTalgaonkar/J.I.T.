#!/bin/bash

# Set the paths to the Npcap Include and Lib directories
NPCAP_INCLUDE_DIR="lib/npcap-sdk-1.15/Include"
NPCAP_LIB_DIR="lib/npcap-sdk-1.15/lib"


GCC_PATH="gcc"  


SOURCE_FILE="source/lib/src/main.c"  
OUTPUT_FILE="build/packet_sniffer.exe"  


mkdir -p build

$GCC_PATH $SOURCE_FILE -o $OUTPUT_FILE -I"$NPCAP_INCLUDE_DIR" -L"$NPCAP_LIB_DIR" -lwpcap -lpacket




if [ $? -eq 0 ]; then
    echo "Compilation successful! Running the program..."
    
    ./$OUTPUT_FILE
else
    echo "Compilation failed."
fi
