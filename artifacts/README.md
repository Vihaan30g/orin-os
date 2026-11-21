# artifacts/

This folder is for build outputs that the report needs but that only exist on the build workstation (`marsuser@marsnew`).

**Status: empty on purpose.** The workstation was not reachable when this repository was assembled. To fill it, run this on the workstation:

```bash
cd ~/orin-os && git pull && ./scripts/collect-artifacts.sh ~/yocto
```

It creates:

| Path | Contents |
|---|---|
| `kernel/kernel.config` | The kernel `.config` used for the image (5.15.199-l4t-r36.5) |
| `dtb/*.dtb` | Device-tree blobs, e.g. `kernel_tegra234-p3737-0000+p3701-0005-nv.dtb` (this name appears in the flash logs) |
| `manifest/package-manifest.txt` | The 853-line package list of the image |
| `licenses/license.manifest`, `licenses/spdx.tar.gz` | Licence list and SPDX data for every package |
| `conf/local.conf`, `conf/bblayers.conf`, `conf/layer-versions.txt` | The real build configuration and layer revisions |

The image file itself (`myrobot-image-*.tegraflash.tar.zst`, about 2.4 GB) is too large for git. Rebuild it with [`../docs/BUILD_PROCESS.md`](../docs/BUILD_PROCESS.md), or publish it as a GitHub Release asset.
