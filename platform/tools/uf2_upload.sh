#!/bin/sh
# Uploads a UF2 file to a Phyduino board that is in BOOTSEL mode (Linux, macOS).
#
# Usage: sh uf2_upload.sh <sketch.uf2> <picotool>
#
# Called by platform.txt after the IDE's 1200-baud touch has rebooted the board
# into BOOTSEL. Retries for up to 15 s; on each try it
#   1. copies the UF2 to the BOOTSEL drive (RPI-RP2 / RP2350) if the desktop has
#      mounted it: needs no permissions set-up, or
#   2. runs "picotool load -x" (Linux needs picotool's udev rules for this).
# The board reboots into the new sketch by itself when the copy completes.
UF2=$1
PICOTOOL=$2

if [ ! -f "$UF2" ]; then
    echo "uf2_upload: file not found: $UF2" >&2
    exit 2
fi

try=0
while [ "$try" -lt 15 ]; do
    for dir in /Volumes/RPI-RP2 /Volumes/RP2350 \
               /media/*/RPI-RP2 /media/*/RP2350 /run/media/*/RPI-RP2 /run/media/*/RP2350 \
               /media/RPI-RP2 /media/RP2350 /mnt/RPI-RP2 /mnt/RP2350; do
        if [ -f "$dir/INFO_UF2.TXT" ]; then
            echo "Copying to BOOTSEL drive $dir ..."
            if cp "$UF2" "$dir/" && sync; then
                echo "Upload complete; the board restarts into the new sketch."
                exit 0
            fi
        fi
    done

    if "$PICOTOOL" load -x "$UF2" >/dev/null 2>&1; then
        echo "Upload complete via picotool."
        exit 0
    fi

    sleep 1
    try=$((try + 1))
done

echo "uf2_upload: no board in BOOTSEL mode found after 15 s." >&2
echo "  - New board, or the sketch crashed: slide SW1 to GND, press reset, then upload again." >&2
echo "  - Or copy the .uf2 file to the RPI-RP2 drive by hand." >&2
exit 1
