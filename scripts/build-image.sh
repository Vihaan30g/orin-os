#!/usr/bin/env bash
# Build the image. Assumes setup-build-env.sh was run and conf/ samples were copied into build/conf.
set -euo pipefail
WORK="${1:-$HOME/yocto}"
cd "$WORK"
# shellcheck disable=SC1091
source poky/oe-init-build-env build
bitbake myrobot-image
echo "Artifacts: $WORK/build/tmp/deploy/images/p3737-0000-p3701-0005/"
