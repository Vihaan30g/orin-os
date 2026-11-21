# orin-os: a custom Yocto Linux for the Jetson AGX Orin

**A lean, hardened, headless Linux image for robotics. It is built with the Yocto Project and runs CUDA, TensorRT and ROS 2 Humble on a Jetson AGX Orin dev kit. A YOLOv8 webcam detector starts at boot and publishes on a ROS 2 topic. A separate writable partition lets you add your own code without rebuilding or reflashing the OS.**

Author: **Vihaan Gupta** · vihaan30g@gmail.com · GitHub [@vihaan30g](https://github.com/vihaan30g)

---

## 1. What this project is, in brief

NVIDIA's stock JetPack image for the Orin is a full Ubuntu desktop. It has 3,192 packages, about 19 GB installed, 53 running services and a 28 s boot. That is a poor fit for a robot that only needs to do a few things reliably. I replaced it with a Yocto-built OS made for deployment:

- **Built from source with Yocto Project 5.0 (scarthgap)**, using `meta-tegra` (L4T R36.5.2, Linux 5.15), `meta-ros` (ROS 2 Humble) and one custom layer, [`meta-myrobot`](meta-myrobot/).
- **GPU stack included:** CUDA 12.6, cuDNN 9.3, TensorRT 10.3 and CUDA-enabled OpenCV 4.9.
- **A real application runs at boot.** `myrobot-yolo.service` reads a USB webcam, runs YOLOv8n through TensorRT (FP16) and publishes `vision_msgs/Detection2DArray` on `/detections`.
- **Headless and hardened:** SSH keys only, no root login, no password login, and unused daemons masked.
- **Wireless:** onboard Wi-Fi works (RTL8822CE), and ROS 2 topics are visible from a laptop with no cable between the two machines.
- **Deploy without reflashing:** `/home/rover` is a separate 32 GB `DATA` partition (`PARTLABEL=DATA`). New ROS 2 packages, models and scripts go there, and an OS update never touches it. The deployment model is described in [section 6](#6-adding-code-without-rebuilding-or-reflashing).

What I did not do is described honestly in [section 7](#7-honest-scope-and-known-limitations). The two main gaps are that this is **not hard real-time** (the kernel is `PREEMPT`, not `PREEMPT_RT`) and that the root filesystem is **not yet read-only**.

## 2. Results

All numbers were measured on the real hardware. They come from the 8b image (flashed 2026-11-19) and the stock baseline (2026-11-05). Full evidence is in [`REPORT.md`](REPORT.md) and the screenshots are in [`docs/TESTS.md`](docs/TESTS.md).

| Metric | Stock JetPack (Ubuntu) | **This OS (Yocto)** | Change |
|---|---|---|---|
| Installed packages | 3,192 | **853** | about 3.7x fewer |
| Installed size | 19,277 MB | **about 4.0 GB** used on `/` | about 4.8x smaller |
| Running services | 53 | **19** | about 2.8x fewer |
| Loaded kernel modules | 160 | **87** | about 1.8x fewer |
| Boot time (to `multi-user.target`) | 28.3 s | **21.3 s**, with detector service started | 25% faster |
| Idle RAM used | about 2.7 GB | **about 1.1 GB**, detector running | about 2.4x less |
| Open network ports | not recorded | **1 TCP (22/ssh)**, plus UDP DDS from the detector | n/a |

| Application result | Value |
|---|---|
| YOLOv8n TensorRT FP16, GPU latency (`trtexec`) | **5.54 ms** (180 queries/s) |
| End-to-end detector rate (capture, inference and publish) | **29.4 fps**, 25.2 ms/frame (camera limited) |
| `/detections` rate seen on the laptop over Wi-Fi | **29.3 Hz**, std dev about 4 ms |
| Person detection score (`ros2 topic echo --once`) | **0.82** (`class_id: person`) |
| Power (`VIN_SYS_5V0`), idle vs detector running | **2.3 W to 3.4 W** at `MODE_30W` |
| Flash time (`initrd-flash`, eMMC) | **about 4 min** |
| Flashes completed successfully | **7 of 7** (`Final status: SUCCESS`) |

Two things I would flag when reading these numbers:

- The stock and Yocto boot times were measured on different days and are not a controlled experiment.
- The boot figure of 21.3 s includes a fix I applied live: masking `systemd-networkd-wait-online`. Before the fix the image took **2 min 11 s** to boot, and [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) explains why.

## 3. Demo videos

| # | What it shows | Link |
|---|---|---|
| 1 | **Wi-Fi working.** The Orin has no cable to the laptop, yet its ROS 2 topics appear on the laptop. | [![Video 1: Wi-Fi and ROS 2 topics](https://img.shields.io/badge/YouTube-Video%201%20(placeholder)-red?logo=youtube)](https://www.youtube.com/watch?v=VIDEO_1_ID) |
| 2 | **AI detector demo.** `ros2 topic echo --once /detections`: with a hand over the camera nothing is detected. With a person in front, a `person` is detected with a high score. | [![Video 2: YOLO detection demo](https://img.shields.io/badge/YouTube-Video%202%20(placeholder)-red?logo=youtube)](https://www.youtube.com/watch?v=VIDEO_2_ID) |

> The links above are placeholders. To fill them in, replace `VIDEO_1_ID` and `VIDEO_2_ID` with the YouTube video IDs.

## 4. What is in this repository

| Path | Contents |
|---|---|
| [`meta-myrobot/`](meta-myrobot/) | The custom Yocto layer: image recipes, partition layout, ROS 2 detector node and services. This is the **build recipe**. |
| [`conf/`](conf/) | Sample `bblayers.conf` and `local.conf`, to reproduce the build. |
| [`scripts/`](scripts/) | `setup-build-env.sh`, `build-image.sh`, `flash.sh`, and `collect-artifacts.sh` (for the kernel `.config`, DTBs and manifests). |
| [`docs/`](docs/) | Build process, layer contents, tests with screenshots, troubleshooting, flash-log notes and the deliverables map. |
| [`REPORT.md`](REPORT.md) | The report: benchmarks, baseline comparison, licence list, limitations. |
| [`evidence/`](evidence/) | Raw flash logs and stock-baseline measurements. |
| [`artifacts/`](artifacts/) | Where the kernel `.config`, DTBs, package manifest and licence manifest belong (see [`artifacts/README.md`](artifacts/README.md)). |
| [`archive/`](archive/) | The older 7a layer copy and the original kickoff planning documents. |

**Where to start reading:**

- Recruiters and professors: this page, then [`REPORT.md`](REPORT.md) and [`docs/TESTS.md`](docs/TESTS.md).
- Engineers who want to rebuild it: [`docs/BUILD_PROCESS.md`](docs/BUILD_PROCESS.md).
- The deliverables map: [`docs/DELIVERABLES.md`](docs/DELIVERABLES.md).

## 5. How it was built

The project went through these stages. The exact commands are in [`docs/BUILD_PROCESS.md`](docs/BUILD_PROCESS.md).

1. **Baseline.** Measured the stock JetPack install (boot time, packages, services, RAM, power) before touching anything, and backed up the eMMC.
2. **Build host.** Ubuntu 22.04 workstation with Yocto scarthgap, `meta-tegra`, `meta-openembedded` and `meta-ros`.
3. **Minimal image.** A `core-image` base with SSH and NetworkManager, flashed over USB recovery mode with `initrd-flash`.
4. **Hardware bring-up.** Fixed the machine definition (`p3737-0000-p3701-0005`, the 64 GB module). This fixed the GPU memory-allocation failure and enabled Wi-Fi (`rtl8822ce`) and the UVC camera.
5. **GPU stack.** Added CUDA, cuDNN and TensorRT, then a TensorRT `trtexec` benchmark.
6. **ROS 2 and the application.** Added ROS 2 Humble and the custom `myrobot_yolo` node as a Yocto recipe, started by systemd.
7. **Partition layout.** Added the `DATA` partition for `/home/rover`, so user data stays separate from the OS.
8. **Hardening and trimming.** SSH key-only login, an unprivileged `rover` user, and masked unused services.
9. **Verification and fixes.** Every test is in [`docs/TESTS.md`](docs/TESTS.md). The bugs I hit are in [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md).

The seven flash runs behind this are summarised in [`docs/FLASH_LOGS.md`](docs/FLASH_LOGS.md).

### The layers

| Layer | Source | What it provides |
|---|---|---|
| `poky` | Yocto Project, scarthgap 5.0.21 | BitBake, OpenEmbedded-Core, the `poky` distro, systemd and base userland |
| `meta-openembedded` | OpenEmbedded | NetworkManager, wpa-supplicant, v4l-utils, many ROS 2 dependencies |
| `meta-tegra` | OE4T, L4T R36.5.2 | Jetson BSP: bootloader, kernel 5.15, NVIDIA OOT modules, CUDA, cuDNN, TensorRT, flashing tools |
| `meta-ros` | OpenEmbedded ROS | ROS 2 Humble packages |
| **`meta-myrobot`** | this repo | Image recipes, partition layout, detector node, services |

Layer revisions are pinned in [`evidence/layer-versions.txt`](evidence/layer-versions.txt). A full description of each layer and what it contributes is in [`docs/LAYERS.md`](docs/LAYERS.md).

## 6. Adding code without rebuilding or reflashing

The OS is split in two:

```
 eMMC (59 GB)
 ├─ boot partitions (A/B kernel, DTB, ESP, recovery)  <- written by flashing
 ├─ APP  /            14.8 GB ext4  <- the OS (Yocto image)       <- replaced by flashing
 └─ DATA /home/rover  32 GB ext4    <- your code, models, workspace  <- survives OS updates
```

- ROS 2 workspaces, models and scripts live in `/home/rover`. The detector keeps its TensorRT engine at `/home/rover/models/yolov8n.engine`. It is built on the board the first time the service starts, and it is not part of the OS image.
- A `rover` user with passwordless `sudo` and SSH key login lets you copy files in from a laptop and run them.
- **Honest caveat:** this works for **your own code and data**. Anything that must live in `/usr` or `/opt/ros` (for example a new system library) is still an image change. A `myrobot-image-dev` variant with a compiler and ROS 2 dev tools is written in the layer but has **not been built or tested**, so on-device `colcon build` is not yet demonstrated here.

The partition layout is defined in [`meta-myrobot/recipes-bsp/tegra-binaries/files/flash_t234_qspi_sdmmc.xml`](meta-myrobot/recipes-bsp/tegra-binaries/files/flash_t234_qspi_sdmmc.xml).

## 7. Honest scope and known limitations

| Item | Status |
|---|---|
| Hard real-time | **No.** The kernel is `PREEMPT` (low-latency), not `PREEMPT_RT`. No latency benchmark such as `cyclictest` was run. |
| Read-only root filesystem | **Not done.** The recipe has a switch (`MYROBOT_RO_ROOT`) that defaults to `0`, and it has not been tested. |
| Developer image (`myrobot-image-dev`) | Written, **never built**. |
| Kernel trimming | **Not done.** The kernel config is NVIDIA's L4T default. The smaller module count comes from not installing modules. |
| Boot fixes applied live | Masking `systemd-networkd` and ordering the detector after `network-online.target` were first applied on the running board. They are now in the layer, but **no image has been rebuilt and re-flashed with them**. The tests in [`docs/TESTS.md`](docs/TESTS.md) used the live-patched system. |
| Wi-Fi driver logging | The `rtl8822ce` out-of-tree driver prints a large amount of debug output to the serial console. `dmesg -n 1` silences it. |
| Camera hot-unplug | If the USB camera is unplugged while the detector runs, the device node can renumber (`video1`/`video2`) and the node does not reopen it. Stop the service, replug the camera and start the service again. |
| System clock | The board has no RTC battery and no network time at first boot, so timestamps in the logs can be wrong. |
| Memory figure | An earlier build read about 3.6 GiB used at idle, and the final one reads about 1.1 GiB. I did not find out what caused the difference, so I quote the final figure with this note. |
| Wi-Fi credentials | Not baked into the image. They are entered on the device with `nmcli`. |

## 8. Quick start

On an Ubuntu 22.04 x86-64 build host with plenty of free disk (several hundred GB is comfortable) and 16 GB of RAM or more:

```bash
git clone https://github.com/vihaan30g/orin-os.git && cd orin-os
./scripts/setup-build-env.sh      # clones poky, meta-openembedded, meta-tegra, meta-ros at the pinned revisions
./scripts/build-image.sh          # bitbake myrobot-image  (several hours on the first build)
./scripts/flash.sh                # put the Orin in recovery mode first, then run initrd-flash
```

Replace the placeholder SSH public key in [`meta-myrobot/recipes-core/images/myrobot-image.bb`](meta-myrobot/recipes-core/images/myrobot-image.bb) with your own before you build. Full steps are in [`docs/BUILD_PROCESS.md`](docs/BUILD_PROCESS.md).

## 9. Licence

The `meta-myrobot` layer is MIT licensed (`meta-myrobot/COPYING.MIT`). The image contains many third-party components under their own licences, such as GPL, LGPL, Apache and NVIDIA's proprietary licence. The list and how it was generated are in [`REPORT.md`](REPORT.md#6-licence-list).
