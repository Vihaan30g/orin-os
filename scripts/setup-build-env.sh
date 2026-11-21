#!/usr/bin/env bash
# Clone the Yocto layers at the exact revisions used for the 8b image.
# Run from the repo root on an Ubuntu 22.04 x86-64 build host.
set -euo pipefail
WORK="${1:-$HOME/yocto}"
mkdir -p "$WORK" && cd "$WORK"
BRANCH=scarthgap
clone() { # name url rev
  [ -d "$1" ] || git clone "$2" "$1"
  git -C "$1" fetch --all -q
  git -C "$1" checkout -q "$3"
}
# Revisions come from evidence/layer-versions.txt (poky, meta-openembedded, meta-tegra, meta-ros)
clone poky              https://git.yoctoproject.org/poky                    9517df0a9af21f1e0f9d6ba5a78c5ae1d7c83184
clone meta-openembedded https://git.openembedded.org/meta-openembedded       2db6aece32c7f8ebf6015b468e1b4e21de072e3b
clone meta-tegra        https://github.com/OE4T/meta-tegra                   6d7cc8f27e8b3bda2abfa68b2fcfe8d6ecf10464
clone meta-ros          https://github.com/ros/meta-ros                      d8b213ec652c7f5ec0fb725cde2d041aaaec3ce5
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ln -sfn "$REPO/meta-myrobot" "$WORK/meta-myrobot"
echo "Layers ready in $WORK (branch family: $BRANCH)."
echo "Next: source poky/oe-init-build-env build, copy conf/*.sample into build/conf (see docs/BUILD_PROCESS.md)."
