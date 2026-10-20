#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RAW_DIR="$ROOT_DIR/assets/music/raw"
OUT_DIR="$ROOT_DIR/assets/music"

mkdir -p "$OUT_DIR"

if ! command -v ffmpeg &>/dev/null; then
    echo "Warning: ffmpeg not found, using existing audio files if present."
    exit 0
fi

for raw_file in "$RAW_DIR"/*.mp3; do
    [ -f "$raw_file" ] || continue
    base_name="$(basename "$raw_file")"
    out_file="$OUT_DIR/$base_name"

    if [ ! -f "$out_file" ] || [ "$raw_file" -nt "$out_file" ]; then
        echo "Converting $base_name with ffmpeg (96kbps, 44.1kHz stereo)..."
        ffmpeg -y -i "$raw_file" -codec:a libmp3lame -b:a 96k -ar 44100 -ac 2 "$out_file"
    fi
done

echo "Audio conversion complete."
