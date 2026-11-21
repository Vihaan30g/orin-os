# Build process, step by step

This describes how the image was built and how to rebuild it. Commands that come straight from the project history are plain. Anything reconstructed, where the original command was not kept, is marked **(reconstructed)**. Check those against your own setup.

## 0. The idea and the goal

**Starting point.** A Jetson AGX Orin dev kit ships with a full Ubuntu-based JetPack image. It works, but it is large and slow to boot, has many services that a robot does not need, and is hard to reproduce exactly.

**Objective.** Build a custom OS for deploying real-time-style robotics workloads on the Orin:

1. small, hardened and reproducible, built from source;
2. CUDA, TensorRT and ROS 2 available, with a real AI application running at boot;
3. **new packages and code can be put on the machine without rebuilding the OS or reflashing the board.**

**What was achieved.** Objectives 1 and 2 are met and measured ([`REPORT.md`](../REPORT.md)). Objective 3 is met for user code and data, which live on the separate `DATA` partition. It is not met for system-level packages, and the on-device developer image was not built. "Real-time" here means low-latency and deterministic start-up, not hard real-time.

## 1. Baseline the stock system

Before changing anything, the stock JetPack install was measured and the eMMC partition table and `/` were backed up (`evidence/stock-baseline/`): package count, running services, modules, installed size, RAM, boot time (`systemd-analyze`), power (`tegrastats`), boot entries (`efibootmgr`, `nvbootctrl`) and the Wi-Fi and USB hardware (`lspci`, `lsusb`).

## 2. Prepare the build host

Workstation `marsuser@marsnew`, Ubuntu 22.04 x86-64. Install the Yocto host packages (the list is in the Yocto Project Quick Build guide for scarthgap), plus `picocom` for the serial console. Automatic mounting of USB storage was disabled on the workstation (recommended) so that the Orin's storage, exposed during flashing, does not get mounted by the desktop.

```bash
./scripts/setup-build-env.sh ~/yocto        # clones the 4 upstream layers at the pinned commits
```

The pinned commits are in [`evidence/layer-versions.txt`](../evidence/layer-versions.txt). Cloning poky over `git://` failed, so use `https://`.

## 3. Configure the build

```bash
cd ~/yocto && source poky/oe-init-build-env build
cp ../orin-os/conf/bblayers.conf.sample conf/bblayers.conf   # (reconstructed) then fix the paths
# merge conf/local.conf.sample into conf/local.conf          # (reconstructed)
```

Settings that matter, all established during the project:

- `MACHINE = "p3737-0000-p3701-0005"`. This is the **64 GB module (SKU 0005)**. Using the devkit machine (SKU 0000) fails the flash check and breaks GPU memory allocation.
- `DISTRO = "poky"`, systemd as init manager.
- CMA raised from 256 MB to 512 MB for the GPU.
- `meta-myrobot` added to `BBLAYERS`. Put your own SSH public key in `myrobot_config` in `myrobot-image.bb`.

The real `local.conf` and `bblayers.conf` were not available when this repo was assembled. They are listed as missing in [`DELIVERABLES.md`](DELIVERABLES.md).

## 4. Build

```bash
./scripts/build-image.sh ~/yocto            # bitbake myrobot-image
```

Output: `build/tmp/deploy/images/p3737-0000-p3701-0005/`, including `myrobot-image-p3737-0000-p3701-0005*.tegraflash.tar.zst` (about 2.4 GB). The image has 853 packages.

If the network drops during fetch, rerun `bitbake`. Completed downloads are kept.

## 5. Flash the Orin eMMC

Only the eMMC is flashed. (A USB-stick root was tried first and then dropped to keep the setup simple.)

1. Put the Orin in **USB recovery mode**: hold FORCE RECOVERY, press RESET, and connect the USB-C port next to the 40-pin header to the workstation. `lsusb` shows `0955:7023`.
2. Extract the bundle into a **new, empty** directory and run `initrd-flash`:

```bash
./scripts/flash.sh ~/yocto
# equivalent to: mkdir ~/flash-8b && tar -C ~/flash-8b -xf <bundle> && cd ~/flash-8b && sudo ./initrd-flash 2>&1 | tee ~/flash-8b-log.txt
```

The run takes about 4 minutes and ends with `Final status: SUCCESS`. See [`FLASH_LOGS.md`](FLASH_LOGS.md). Seven flashes were done in total. The flash tool wipes and repartitions the eMMC (`DATA` included), so back up `/home/rover` first.

## 6. First boot

Connect the serial console (USB-C debug port) with `picocom -b 115200 /dev/ttyACM0` and log in as `root` with an empty password. The root account has no SSH access.

Connect to Wi-Fi on the device and check:

```bash
nmcli device wifi connect "<SSID>" password "<PASSWORD>"
ip a show wlP1p1s0
dmesg -n 1        # silences the rtl8822ce debug flood on the serial console
```

The detector service builds the TensorRT engine on first start, then publishes. Then from a laptop with ROS 2 Humble, on the same Wi-Fi:

```bash
export ROS_DOMAIN_ID=42 RMW_IMPLEMENTATION=rmw_fastrtps_cpp
source /opt/ros/humble/setup.bash
ros2 topic list                       # shows /detections
ros2 topic echo /detections --once
```

For SSH, see [`TESTS.md`](TESTS.md) (key-only login as `rover`). After every reflash run `ssh-keygen -R <ip>`.

## 7. Deploying new code without reflashing

```bash
# From the laptop: copy a ROS 2 workspace or script to the data partition
rsync -av ./my_pkg rover@<orin-ip>:/home/rover/ros2_ws/
ssh rover@<orin-ip> 'source /opt/ros/humble/setup.sh && ...'
```

- Python packages and prebuilt ROS 2 packages (compiled for aarch64) can be dropped into `/home/rover` and run.
- Building C++ on the board needs the developer image (`myrobot-image-dev`), which is **not built**.
- Anything that must go into the OS itself needs a new image. Because `/home/rover` is a separate partition, you can reflash `APP` only in principle, but `initrd-flash` as used here repartitions the whole eMMC and wipes `DATA`.

## 8. Collect the artifacts

On the workstation, after a build:

```bash
./scripts/collect-artifacts.sh ~/yocto      # kernel .config, DTBs, manifests, licences, real conf files
```

## 9. Fixes that were made along the way

See [`TROUBLESHOOTING.md`](TROUBLESHOOTING.md). Three matter most for anyone rebuilding: the machine definition (SKU 0005), `tensorrt-plugins` in the image, and the boot-time fix for `systemd-networkd-wait-online`.
