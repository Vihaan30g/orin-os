# Deliverables map

| Required deliverable | Where it is | Status |
|---|---|---|
| **Bootable image or reproducible build recipe** | Recipe: [`meta-myrobot/`](../meta-myrobot/), with [`scripts/`](../scripts/), [`conf/`](../conf/) and the pinned layer revisions in [`evidence/layer-versions.txt`](../evidence/layer-versions.txt). Steps: [`BUILD_PROCESS.md`](BUILD_PROCESS.md). Seven successful flashes are logged in [`evidence/flash-logs/`](../evidence/flash-logs/). | Recipe: **complete**. The image file is not in git (about 2.4 GB). `conf/*.sample` are reconstructed, and the real `local.conf` and `bblayers.conf` are still to be added. |
| **Kernel `.config`** | `artifacts/kernel/kernel.config` | **Missing.** Needs the workstation (`scripts/collect-artifacts.sh`). |
| **Device tree files** | `artifacts/dtb/` | **Missing.** Same script. The flash logs name the DTB that was written (`kernel_tegra234-p3737-0000+p3701-0005-nv.dtb`, 254,784 bytes). |
| **Package manifest** | `artifacts/manifest/package-manifest.txt` | **Missing.** Only the count (853 lines) is recorded, in screenshot [`19`](images/19-manifest-layer-versions.png). |
| **Git repository with scripts and configurations** | This repository | **Done.** |
| **Report with benchmarks** | [`REPORT.md`](../REPORT.md), with the evidence in [`TESTS.md`](TESTS.md) | **Done.** |
| **Licence list** | [`REPORT.md` section 6](../REPORT.md#6-licence-list): summary and generation method | **Partial.** The per-package list needs `license.manifest` from the workstation. |
| Demo videos | Placeholders in the [README](../README.md#3-demo-videos) | Links to be filled in. |
