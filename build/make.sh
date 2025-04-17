#!/bin/bash

# Set paths for Npcap (C program)
NPCAP_INCLUDE_DIR="lib/npcap-sdkit/Include"
NPCAP_LIB_DIR="lib/npcap-sdkit/Lib/x64"
GCC_PATH="x86_64-w64-mingw32-gcc"
C_SOURCE_FILE="source/capture/src/main.c"
C_OUTPUT_FILE="build/packet_sniffer.exe"

# Set paths for Go program
GO_SOURCE_FILE="source/backend/main/main.go"
GO_OUTPUT_FILE="build/packet_processor.exe"

# Create build directory if it doesn't exist
mkdir -p build

# Compile the C program (packet sniffer)
echo "Compiling C program..."
$GCC_PATH "$C_SOURCE_FILE" -o "$C_OUTPUT_FILE" -I"$NPCAP_INCLUDE_DIR" -L"$NPCAP_LIB_DIR" -lwpcap -lPacket -lws2_32 -lshell32

# Check if C compilation succeeded
if [ $? -ne 0 ]; then
    echo "C compilation failed."
    exit 1
fi
echo "C compilation successful!"

# Compile the Go program (packet processor)
echo "Compiling Go program..."
GOOS=windows go build -o "$GO_OUTPUT_FILE" "$GO_SOURCE_FILE"

# Check if Go compilation succeeded
if [ $? -ne 0 ]; then
    echo "Go compilation failed."
    exit 1
fi
echo "Go compilation successful!"

# Run the C program (it will launch the Go program)
echo "Running C program (will launch Go program)..."
./"$C_OUTPUT_FILE"
C_EXIT_STATUS=$?

# Check exit status
if [ $C_EXIT_STATUS -eq 0 ]; then
    echo "C program executed successfully!"
else
    echo "Execution failed. C program exit status: $C_EXIT_STATUS"
    exit 1
fi