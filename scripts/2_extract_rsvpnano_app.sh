#!/bin/bash
# Step 2: Extract just the rsvpnano app binary from the full backup.
# This creates the file we'll flash to ota_1 (offset 0x210000) later.
#
# rsvpnano's default app partition starts at 0x10000 (standard Arduino layout).
# We extract 2MB from that address — more than enough for any rsvpnano build.

set -e

BACKUP_DIR="$(dirname "$0")/../backups"
LATEST_BACKUP=$(ls -t "$BACKUP_DIR"/rsvpnano_full_backup_*.bin 2>/dev/null | head -1)

if [ -z "$LATEST_BACKUP" ]; then
    echo "Error: no backup found in $BACKUP_DIR"
    echo "Run 1_backup_rsvpnano.sh first."
    exit 1
fi

OUTPUT="$BACKUP_DIR/rsvpnano_app.bin"

echo "Extracting rsvpnano app from: $LATEST_BACKUP"
# rsvpnano uses standard partition layout: app starts at 0x10000
# We grab 0x1F0000 bytes (1.9MB) = our ota_1 partition size
dd if="$LATEST_BACKUP" of="$OUTPUT" bs=1 skip=$((0x10000)) count=$((0x1F0000)) 2>/dev/null

echo "✓ Extracted to: $OUTPUT"
echo "  This will be flashed to ota_1 (offset 0x210000) in step 4."
