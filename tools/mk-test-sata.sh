#!/usr/bin/env bash
# mk-test-sata.sh — create a temporary raw SATA test disk image
# Usage: mk-test-sata.sh <output-path> [size-mb]
#
# Requires: qemu-img
# No root/sudo required. The caller deletes the image after use.

set -euo pipefail

OUT="${1:?Usage: mk-test-sata.sh <output-path> [size-mb]}"
SIZE_MB="${2:-8}"

echo "  SATA-IMG creating ${SIZE_MB}MB SATA test disk -> ${OUT}"

# Create a sparse raw disk image — Q35's ICH9-AHCI presents it
# as a SATA device visible to the AHCI driver.
qemu-img create -f raw "$OUT" "${SIZE_MB}M"

echo "  SATA-IMG ${OUT} ready (${SIZE_MB}MB raw)"
