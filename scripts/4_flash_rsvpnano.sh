#!/bin/bash
# Step 4: Flash the extracted rsvpnano binary to ota_1 (offset 0x210000).
# Run this after step 3.

set -e
SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PORT=${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}

RSVP_BIN="$PROJECT_ROOT/backups/rsvpnano_app.bin"

if [ ! -f "$RSVP_BIN" ]; then
    echo "Error: $RSVP_BIN not found."
    echo "Run 2_extract_rsvpnano_app.sh first."
    exit 1
fi

if [ -z "$PORT" ]; then
    echo "Error: no USB serial port found."
    echo "Usage: $0 [/dev/cu.usbmodemXXXX]"
    exit 1
fi

echo "=== Flashing RSVP Nano to ota_1 (0x210000) ==="
echo "Put device in flash mode (hold BOOT, press/release RESET, release BOOT)"
echo "Press Enter when ready..."
read

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
    write_flash \
    --flash_mode qio \
    --flash_size 8MB \
    0x210000 "$RSVP_BIN"

echo ""
echo "✓ RSVP Nano flashed to ota_1."
echo ""
echo "=== All done! ==="
echo "Power cycle the device to reach the boot launcher:"
echo "  • Release BOOT button → Catchphrase"
echo "  • Hold BOOT button   → RSVP Nano"
