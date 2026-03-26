#!/usr/bin/env bash
# mk-test-ata.sh — create a temporary MBR+ext2 ATA test disk image
# Usage: mk-test-ata.sh <output-path> [size-mb]
#
# No root/sudo required. The caller deletes the image after use.

set -euo pipefail

OUT="${1:?Usage: mk-test-ata.sh <output-path> [size-mb]}"
SIZE_MB="${2:-32}"

echo "  ATA-IMG creating ${SIZE_MB}MB test disk -> ${OUT}"

# Create raw image
dd if=/dev/zero of="$OUT" bs=1M count="$SIZE_MB" status=none

# Build an MBR with a single Linux (0x83) partition starting at sector 2048
echo ",,83;" | sfdisk --quiet "$OUT" 2>/dev/null

# Create an ext2 filesystem image for the partition content
# Partition starts at sector 2048 (1MB), so partition size = total - 1MB
PART_SIZE_KB=$(( (SIZE_MB - 1) * 1024 ))
PART_IMG="${OUT}.part"

dd if=/dev/zero of="$PART_IMG" bs=1K count="$PART_SIZE_KB" status=none
mkfs.ext2 -q -F "$PART_IMG"

# Write the ext2 partition into the disk image at the correct offset
dd if="$PART_IMG" of="$OUT" bs=512 seek=2048 conv=notrunc status=none
rm -f "$PART_IMG"

echo "  ATA-IMG ${OUT} ready (MBR + ext2)"
