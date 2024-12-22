#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <base_directory>"
    exit 1
fi

BASE_DIR="$1"
PROGRAM="$RECORDER_INSTALL_PATH/bin/conflict-detector"

if [[ ! -x "$PROGRAM" ]]; then
    echo "Program $PROGRAM not found or is not executable."
    exit 1
fi

for dir in "$BASE_DIR"/*/; do
    if [ -d "$dir" ]; then
        echo "Entering directory: $dir"
        cd "$dir"
        $PROGRAM "$dir"
        echo "command: $PROGRAM $dir"
        echo "finished"
        cd ..
    fi
done