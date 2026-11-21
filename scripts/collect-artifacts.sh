#!/usr/bin/env bash
# Run ON THE BUILD WORKSTATION (marsuser@marsnew) after a successful build.
# Copies the kernel .config, device-tree blobs, package/licence manifests and the
# layer revisions into ./artifacts so they can be committed.
set -euo pipefail
WORK="${1:-$HOME/yocto}"
OUT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/artifacts"
DEPLOY="$WORK/build/tmp/deploy/images/p3737-0000-p3701-0005"
mkdir -p "$OUT"/{kernel,dtb,manifest,licenses,conf}
# Kernel config (from the built kernel; path glob because the work dir name has the version in it)
cfg="$(ls -t "$WORK"/build/tmp/work/p3737_0000_p3701_0005-poky-linux/linux-jammy-nvidia-tegra/*/build/.config 2>/dev/null | head -1 || true)"
[ -n "$cfg" ] && cp "$cfg" "$OUT/kernel/kernel.config" || echo "kernel .config not found (rm_work may have removed it: bitbake -c kernel_configme -f virtual/kernel then rerun)"
# Device trees
cp "$DEPLOY"/*.dtb "$OUT/dtb/" 2>/dev/null || echo "no .dtb in deploy dir"
cp "$DEPLOY"/*.dtbo "$OUT/dtb/" 2>/dev/null || true
# Package manifest + build info
cp "$DEPLOY"/myrobot-image-p3737-0000-p3701-0005.rootfs.manifest "$OUT/manifest/package-manifest.txt"
cp "$DEPLOY"/myrobot-image-p3737-0000-p3701-0005.rootfs.testdata.json "$OUT/manifest/" 2>/dev/null || true
# Licences (create-spdx is enabled by default in poky; adds .spdx.json under tmp/deploy/spdx)
LIC="$(ls -dt "$WORK"/build/tmp/deploy/licenses/myrobot-image-p3737-0000-p3701-0005* 2>/dev/null | head -1 || true)"
[ -n "$LIC" ] && cp "$LIC/license.manifest" "$OUT/licenses/license.manifest" || echo "license.manifest not found"
[ -d "$WORK/build/tmp/deploy/spdx" ] && tar -C "$WORK/build/tmp/deploy" -czf "$OUT/licenses/spdx.tar.gz" spdx || true
# Build configuration
cp "$WORK/build/conf/local.conf" "$WORK/build/conf/bblayers.conf" "$OUT/conf/"
# Layer revisions, including meta-myrobot
for d in poky meta-openembedded meta-tegra meta-ros; do echo "$d"; git -C "$WORK/$d" rev-parse HEAD; done > "$OUT/conf/layer-versions.txt"
echo "meta-myrobot (this repo)" >> "$OUT/conf/layer-versions.txt"; git -C "$OUT/.." rev-parse HEAD >> "$OUT/conf/layer-versions.txt"
ls -R "$OUT" | head -50
