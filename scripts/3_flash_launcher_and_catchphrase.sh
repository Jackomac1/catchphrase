#!/bin/bash
# Step 3: Write the new partition table, flash the launcher to factory (0x10000),
# and flash Catchphrase to ota_0 (0x50000).
#
# Prerequisites:
#   - PlatformIO CLI installed (pio)
#   - Device in flash mode (hold BOOT, press/release RESET, release BOOT)
#
# This ERASES the existing rsvpnano firmware. Make sure you ran step 1 first.

set -e
SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PORT=${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}

if [ -z "$PORT" ]; then
    echo "Error: no USB serial port found."
    echo "Usage: $0 [/dev/cu.usbmodemXXXX]"
    exit 1
fi

echo "=== Building Launcher ==="
cd "$PROJECT_ROOT/launcher"
pio run -e launcher

echo ""
echo "=== Building Catchphrase ==="
cd "$PROJECT_ROOT"
pio run -e waveshare_esp32s3_usb_msc

echo ""
echo "=== Flashing partition table + launcher + Catchphrase ==="
echo "Put device in flash mode now (hold BOOT, press/release RESET, release BOOT)"
echo "Press Enter when ready..."
read

LAUNCHER_BIN="$PROJECT_ROOT/launcher/.pio/build/launcher/firmware.bin"
CATCHPHRASE_BIN="$PROJECT_ROOT/.pio/build/waveshare_esp32s3_usb_msc/firmware.bin"
PARTITIONS_BIN="$PROJECT_ROOT/.pio/build/waveshare_esp32s3_usb_msc/partitions.bin"
BOOTLOADER_BIN="$PROJECT_ROOT/.pio/build/waveshare_esp32s3_usb_msc/bootloader.bin"

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
    write_flash \
    --flash_mode qio \
    --flash_size 16MB \
    --erase-all \
    0x0000   "$BOOTLOADER_BIN" \
    0x8000   "$PARTITIONS_BIN" \
    0xe000   "$PROJECT_ROOT/scripts/blank_otadata.bin" \
    0x10000  "$LAUNCHER_BIN" \
    0x50000  "$CATCHPHRASE_BIN"

echo ""
echo "✓ Launcher and Catchphrase flashed."
echo "  RSVP Nano slot (ota_1) is empty — run step 4 to flash it."
