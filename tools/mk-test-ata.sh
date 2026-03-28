#!/usr/bin/env bash
# mk-test-ata.sh — create a temporary MBR+ext2 ATA test disk image
# Usage: mk-test-ata.sh <output-path> [size-mb]
#
# Requires: qemu-img, sfdisk, mkfs.ext2
# No root/sudo required. The caller deletes the image after use.

set -euo pipefail

OUT="${1:?Usage: mk-test-ata.sh <output-path> [size-mb]}"
SIZE_MB="${2:-32}"

echo "  ATA-IMG creating ${SIZE_MB}MB test disk -> ${OUT}"

# Create a sparse raw disk image
qemu-img create -f raw "$OUT" "${SIZE_MB}M"

# Write an MBR with a single Linux (0x83) partition starting at sector 2048
echo ",,83;" | sfdisk --quiet "$OUT" 2>/dev/null

# Create and format the ext2 partition image
# Partition starts at sector 2048 (1 MB offset), so partition size = total - 1 MB
PART_SIZE_MB=$(( SIZE_MB - 1 ))
PART_IMG="${OUT}.part"
qemu-img create -f raw "$PART_IMG" "${PART_SIZE_MB}M"
mkfs.ext2 -q -F "$PART_IMG"

# Inject the ext2 filesystem into the disk image at the partition offset
dd if="$PART_IMG" of="$OUT" bs=512 seek=2048 conv=notrunc status=none
rm -f "$PART_IMG"

echo "  ATA-IMG ${OUT} ready (MBR + ext2)"
