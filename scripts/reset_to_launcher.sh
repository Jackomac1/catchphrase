#!/bin/bash
# Utility: erase the OTA data partition so the launcher runs on next boot.
# Use this if you're stuck in RSVP Nano and want to get back to the launcher
# without a full reflash.

set -e
PORT=${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}

if [ -z "$PORT" ]; then
    echo "Error: no USB serial port found."
    echo "Usage: $0 [/dev/cu.usbmodemXXXX]"
    exit 1
fi

echo "Erasing OTA data at 0xe000 (8KB) — launcher will run on next power cycle."
echo "Put device in flash mode (hold BOOT, press/release RESET, release BOOT)"
echo "Press Enter when ready..."
read

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
    erase_region 0xe000 0x2000

echo ""
echo "✓ OTA data cleared. Power cycle the device — the launcher will appear."
