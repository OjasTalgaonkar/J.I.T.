#!/bin/bash
set -u

PROJECT_NAME="JIT"

# Create the subfolders for the C code, Go code, and Python GUI
mkdir -p ../src/c src/go src/python


mkdir -p ../src/c/include ../src/c/src ../src/go/main ../src/go/utils ../src/python/gui ../src/python/utils


touch ../src/c/Makefile


touch ../src/c/src/main.c


touch ../src/go/main/main.go


touch ../src/python/gui/main.py

# Output folder structure
echo "Folder structure for $PROJECT_NAME has been created!"
echo "Project structure:"
