#!/usr/bin/env bash
# Regenerate the ADTS fixtures in tests/data/ (needs ffmpeg and the FAAC frontend).
# Synthetic, deterministic sources only: no third-party audio is committed.
# LC streams come from ffmpeg's native AAC encoder (what Home Assistant's
# ffmpeg proxy produces); the HE-AAC v1 stream comes from FAAC.
# usage: scripts/gen_adts_fixtures.sh [path/to/faac]
set -euo pipefail
FAAC=${1:-faac}
OUT="$(cd "$(dirname "$0")/.." && pwd)/tests/data"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"

# Left: 440 Hz tone, right: seeded pink noise (exercises M/S, PNS, short blocks).
stereo_src() { # rate seconds out.wav
  ffmpeg -v error -y -f lavfi -i "sine=f=440:r=$1:d=$2" -f lavfi -i "anoisesrc=c=pink:a=0.2:r=$1:d=$2:seed=1" \
    -filter_complex "amerge=inputs=2" -ar "$1" -c:a pcm_s16le "$3"
}
stereo_src 44100 1.5 "$TMP/st44.wav"
ffmpeg -v error -y -i "$TMP/st44.wav" -c:a aac -b:a 64k -f adts "$OUT/lc_stereo_44k1.aac"
ffmpeg -v error -y -f lavfi -i "sine=f=300:r=16000:d=1.5" -c:a aac -b:a 24k -f adts "$OUT/lc_mono_16k.aac"
"$FAAC" -b 32 --profile he-v1 --overwrite -o "$OUT/he_v1_stereo_44k1.aac" "$TMP/st44.wav" >/dev/null
ls -l "$OUT"
