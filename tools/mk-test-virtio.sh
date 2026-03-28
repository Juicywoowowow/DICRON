#!/usr/bin/env bash
# mk-test-virtio.sh — create a temporary raw virtio-blk test disk image
# Usage: mk-test-virtio.sh <output-path> [size-mb]
#
# Requires: qemu-img
# No root/sudo required. The caller deletes the image after use.

set -euo pipefail

OUT="${1:?Usage: mk-test-virtio.sh <output-path> [size-mb]}"
SIZE_MB="${2:-8}"

echo "  VIO-IMG creating ${SIZE_MB}MB virtio-blk test disk -> ${OUT}"

# Create a sparse raw disk image — virtio-blk needs nothing more than a
# plain raw file; QEMU presents it as a sector-addressable block device.
qemu-img create -f raw "$OUT" "${SIZE_MB}M"

echo "  VIO-IMG ${OUT} ready (${SIZE_MB}MB raw)"
