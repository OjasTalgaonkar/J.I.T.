#!/bin/bash

# Set the paths to the Npcap Include and Lib directories
NPCAP_INCLUDE_DIR="lib/npcap-sdkit/Include"
NPCAP_LIB_DIR="lib/npcap-sdkit/Lib/x64"
GCC_PATH="x86_64-w64-mingw32-gcc"
SOURCE_FILE="source/capture/src/main.c"
OUTPUT_FILE="build/packet_sniffer.exe"

# Create build directory if it doesn't exist
mkdir -p build

# Compile with Winsock library (libraries after source file)
$GCC_PATH "$SOURCE_FILE" -o "$OUTPUT_FILE" -I"$NPCAP_INCLUDE_DIR" -L"$NPCAP_LIB_DIR" -lwpcap -lPacket -lws2_32

# Check if compilation succeeded
if [ $? -eq 0 ]; then
    echo "Compilation successful! Running the program..."
    ./"$OUTPUT_FILE"
else
    echo "Compilation failed."
fi