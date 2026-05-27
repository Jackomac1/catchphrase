#!/bin/bash
# Step 1: Back up the current rsvpnano firmware before touching anything.
# Run this FIRST while rsvpnano is still the only firmware on the device.

set -e

PORT=${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}
if [ -z "$PORT" ]; then
    echo "Error: no USB serial port found. Plug in the device and try again."
    echo "Usage: $0 [/dev/cu.usbmodemXXXX]"
    exit 1
fi

BACKUP_DIR="$(dirname "$0")/../backups"
mkdir -p "$BACKUP_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/rsvpnano_full_backup_$TIMESTAMP.bin"

echo "Backing up full 16MB flash from $PORT → $BACKUP_FILE"
echo "Put device in flash mode: hold BOOT button, unplug USB, replug USB, release BOOT"
echo "Press Enter when ready..."
read

python3 -m esptool --chip esp32s3 --port "$PORT" \
    read_flash 0x0 0x1000000 "$BACKUP_FILE"

echo ""
echo "✓ Backup saved to: $BACKUP_FILE"
echo "  Keep this file safe — it lets you fully restore rsvpnano if anything goes wrong."
