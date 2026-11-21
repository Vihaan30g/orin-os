#!/usr/bin/env bash
# Flash the Orin eMMC over USB. Put the board in recovery mode first
# (hold FORCE RECOVERY, press RESET, connect the USB-C port next to the 40-pin header).
# Usage: ./flash.sh <work dir>   (default ~/yocto)
set -euo pipefail
WORK="${1:-$HOME/yocto}"
IMG="$WORK/build/tmp/deploy/images/p3737-0000-p3701-0005"
BUNDLE="$(ls -t "$IMG"/myrobot-image-p3737-0000-p3701-0005*.tegraflash.tar.zst | head -1)"
OUT="$HOME/flash-$(date +%Y%m%d-%H%M)"
[ ! -e "$OUT" ] || { echo "$OUT already exists, refusing to overwrite"; exit 1; }
mkdir "$OUT"
tar -C "$OUT" -xf "$BUNDLE"
cd "$OUT"
lsusb | grep -qi "0955:7023" || echo "WARNING: no Orin in recovery mode seen by lsusb (0955:7023)"
sudo ./initrd-flash 2>&1 | tee "$OUT.log"
