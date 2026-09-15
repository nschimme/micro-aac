#!/usr/bin/env bash
set -euo pipefail

echo "=== Running ESP32-S3 QEMU AAC Encoder Test ==="

# Check if QEMU ESP32-S3 binary is installed
if ! command -v qemu-system-xtensa &> /dev/null; then
    echo "qemu-system-xtensa not found, skipping local QEMU execution"
    exit 0
fi

echo "Building example for ESP32-S3..."
cd examples/encode_benchmark
pio run -e esp32s3

echo "QEMU build ready."
