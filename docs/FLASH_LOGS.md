# Flash logs

Every flash used NVIDIA's `initrd-flash` flow from the `tegraflash` bundle that `meta-tegra` produces. The Orin is put in USB recovery mode and then:

1. the host signs the binaries,
2. boots the Jetson over RCM,
3. sends the flash sequence and the Jetson exposes its storage over USB,
4. the partition table is created and the images are written,
5. the host waits for the final status.

The raw logs are in [`evidence/flash-logs/`](../evidence/flash-logs/). Each run folder holds the host log (`log.initrd-flash.<date>`) and the device-side logs (`bootloader.log`, `custom-extra.log`, `export-mmcblk0.log`). A copy of the terminal output sits beside each folder as `flash-<id>-log.txt`.

| Run | Date and time (+05:30) | Machine | Root image written | Result | What changed or why |
|---|---|---|---|---|---|
| 5a | 2026-11-08 05:22 | not in host log | 30,064,771,072 bytes (28 GiB) | SUCCESS | Early iteration, no `DATA` partition |
| 5b | 2026-11-08 07:04 | not in host log | 28 GiB | SUCCESS | Early iteration, no `DATA` partition |
| 5c | 2026-11-08 08:50 | not in host log | 28 GiB | SUCCESS | Early iteration, no `DATA` partition |
| 5d | 2026-11-08 11:26 | not in host log | 28 GiB | SUCCESS | Early iteration, no `DATA` partition (an earlier attempt at 11:16 is also logged) |
| 6a | n/a | n/a | n/a | not flashed | Folder is empty |
| 6b | 2026-11-08 13:10 | not in host log | 16 GiB (17,179,869,184) | SUCCESS | First log with the `DATA` partition; root image is 16 GiB |
| 7a | 2026-11-09 15:11 | `p3737-0000-p3701-0005` | 16 GiB | SUCCESS | Adds the `DATA` partition (`/home/rover`), ROS 2 and the detector. Flash took about 4 min. |
| 8a | n/a | n/a | n/a | not flashed | Fix build; superseded by 8b |
| **8b** | **2026-11-19 09:49:36 to 09:53:52** | `p3737-0000-p3701-0005` | 16 GiB | **SUCCESS** | **Final image.** Fixes the `install` problem in `myrobot-datadirs.service`, masks unused daemons, adds `iproute2-ss`, and fixes the SSH `AuthorizedKeysFile` order. |

**Reading the 8b log.** The key lines (screenshot [`01`](images/01-flash-8b-success.png)) are:

- `Machine: p3737-0000-p3701-0005` and `Rootfs device: mmcblk0p1`: the correct 64 GB module definition, and the eMMC.
- `Creating partitions`: 16 partitions are listed, including `[16] name=DATA size=67108864 sectors` (32 GiB), and `[01] name=APP (fills to end)`.
- `Writing myrobot-image.ext4 (size=17179869184) to /dev/sdb1 (size=27660369408)`: the 16 GiB root image goes into a 25.8 GiB `APP` partition.
- `Writing kernel_tegra234-p3737-0000+p3701-0005-nv.dtb (size=254784)` to both the A and B kernel-DTB partitions, and `boot.img` (48,211,968 bytes) to both the A and B kernel partitions.
- `Final status: SUCCESS`.

**Honest notes.**

- The first image was built for the wrong machine definition (`jetson-agx-orin-devkit`, SKU 0000). The board is SKU 0005, so the flash check failed. Fixing this took two steps: a temporary `TEGRA_BOARDSKU` override, then the proper `MACHINE=p3737-0000-p3701-0005`. The proper machine definition also fixed the GPU memory-allocation (CMA / comptags) failure. See [`TROUBLESHOOTING.md`](TROUBLESHOOTING.md).
- The host logs of runs 5a to 6b do not contain a `Machine:` line, and the 5x runs have no `DATA` partition. Which machine definition each used is therefore not recorded in the logs.
- The 7a log is the older build. Its flash screenshot is [`h1`](images/h1-flash-7a-success.png).
