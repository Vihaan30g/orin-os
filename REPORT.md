# Project report: custom Yocto OS for the Jetson AGX Orin

**Author:** Vihaan Gupta (vihaan30g@gmail.com, GitHub vihaan30g)
**Hardware:** Jetson AGX Orin dev kit, 64 GB module (p3701-0005 on p3737-0000), USB webcam, onboard Wi-Fi (RTL8822CE)
**Software:** Yocto Project 5.0 scarthgap (Poky 5.0.21), meta-tegra L4T R36.5.2, kernel 5.15.199-l4t-r36.5, ROS 2 Humble, CUDA 12.6, cuDNN 9.3, TensorRT 10.3
**Image tested:** 8b, flashed 2026-11-19

## 1. Summary

A custom Linux image was built from source for the Orin. It is smaller, boots faster and runs fewer services than the stock JetPack image. It runs a TensorRT YOLOv8n person detector at boot and publishes the results over ROS 2, reachable from a laptop over Wi-Fi. User code and data live on a separate partition that an OS update does not touch.

It is **not hard real-time**, and the root filesystem is **not read-only**. Section 7 lists what remains.

## 2. Objectives and outcome

| Objective | Outcome |
|---|---|
| Custom OS for robotics deployment, built reproducibly from source | **Met.** Recipe in `meta-myrobot/`, layer commits pinned. The real `local.conf` is still to be added. |
| Lean and hardened compared with stock | **Met**, see section 3 and the SSH checks in [`docs/TESTS.md`](docs/TESTS.md). |
| CUDA, TensorRT and ROS 2 with a real AI workload | **Met.** YOLOv8n at 29.4 fps end to end, 5.54 ms GPU time. |
| New packages and code added without rebuilding or reflashing | **Partly met.** User code and data go on the `DATA` partition. System packages and on-device C++ builds need an image change or the unbuilt dev image. |
| Real-time | **Not met** in the hard sense. Kernel is `PREEMPT`, not `PREEMPT_RT`, and no latency test was run. |

## 3. Benchmarks

### 3.1 System footprint: stock vs custom

| Metric | Stock JetPack | Custom (8b) | Source |
|---|---|---|---|
| Installed packages | 3,192 (`dpkg`) | 853 (image manifest) | `evidence/stock-baseline/phase0a.txt`; screenshot 19 |
| Installed size | 19,277 MB | 4.0 GB used on `/` (14.8 GB partition) | `phase0b.txt`; screenshot 05 |
| Running services | 53 | 19 | `phase0a.txt`; screenshot 10 |
| Kernel modules loaded | 160 | 87 | `phase0a.txt`; screenshot 18 |
| Boot, kernel plus userspace | 8.2 s + 20.1 s = **28.3 s** | 10.8 s + 10.5 s = **21.3 s** | `phase0a.txt`; screenshot 23 |
| RAM used, idle | about 2.7 GB (2,712 MB) | about 1.1 GiB, detector running | `phase0a.txt`; screenshot 18 |
| Kernel | 5.15.199-tegra | 5.15.199-l4t-r36.5-1050.50, PREEMPT | |

Caveats:

- The two systems were measured on different days, and not in a controlled test.
- The custom image's boot time includes the fix described in section 3.3, which was applied on the live system.
- The stock "installed size" is the package database total (`dpkg`), and the custom one is the used space on `/`. They are close but not the same measure.
- Stock RAM was read on the full Ubuntu desktop system. The custom figure was derived as `MemTotal - MemFree - Buffers - Cached` from `/proc/meminfo`. An earlier custom build showed about 3.6 GiB used, and I did not find the cause of the difference.

### 3.2 AI workload

| Measurement | Result | Source |
|---|---|---|
| TensorRT engine, YOLOv8n FP16, 640x640: GPU compute time | 5.54 ms mean (5.51 to 5.56 ms), 180 qps | `trtexec --loadEngine`, screenshot 12 |
| `trtexec` host latency | 6.05 ms mean, 6.07 ms p99 | same |
| Detector end to end | 29.4 fps, 25.2 ms/frame (capture excluded) | node log, screenshot 14 |
| Topic rate on the board | 29.3 Hz | `ros2 topic hz`, screenshot 15 |
| Topic rate on the laptop over Wi-Fi | not measured with numbers (shown in video 1) | |
| Detection example | `person`, score 0.8232 | `ros2 topic echo --once`, screenshot 16 |

The end-to-end rate is about 29 to 30 fps because it is limited by the webcam's frame rate and not by the GPU, which needs about 5.5 ms per frame. Power mode was `MODE_30W`.

### 3.3 Boot time

| State | Kernel | Userspace | Total |
|---|---|---|---|
| Stock JetPack | 8.2 s | 20.1 s | 28.3 s |
| Yocto 8b, as flashed | 10.0 s | 2 min 1.1 s | **2 min 11.1 s** |
| Yocto 8b with `systemd-networkd` masked (live) | 10.8 s | 10.5 s | **21.3 s** |

The slow case was `systemd-networkd-wait-online` waiting for a network that NetworkManager manages. The fix is now in the layer source, and no rebuilt image was flashed to confirm it.

### 3.4 Power

| State | `VIN_SYS_5V0` | RAM (tegrastats) |
|---|---|---|
| Detector stopped | 2,307 mW | 1,012 MB |
| Detector running | 3,416 mW | 1,117 MB |

The stock system showed about 4.9 to 5.1 W under a light test load (`phase0a.txt`). That was not a clean idle measurement, so I do not claim a power saving from it.

### 3.5 Flash

Seven eMMC flashes completed with `Final status: SUCCESS`. The final one took 4 min 16 s (09:49:36 to 09:53:52). See [`docs/FLASH_LOGS.md`](docs/FLASH_LOGS.md).

## 4. Security posture

Checked on the device ([`docs/TESTS.md`](docs/TESTS.md), section 3):

- SSH accepts public keys only. Root login, password login and empty passwords are refused.
- The `rover` user has passwordless `sudo` by design, for a single-user robot. This is a trade-off. Remove it for any shared or exposed deployment.
- The only TCP port open is 22. The detector opens UDP ports for DDS.
- Unused daemons (avahi, bluetooth, ofono, rpcbind, dnsmasq) are masked.
- The serial console login is root with an empty password. Anyone with physical access to the debug port has root, and this was left for bring-up.
- ROS 2 DDS traffic is not encrypted or authenticated (no SROS 2). Anyone on the same Wi-Fi network and domain ID can read the topics.

## 5. Deliverables

See [`docs/DELIVERABLES.md`](docs/DELIVERABLES.md).

## 6. Licence list

**Status:** the per-package licence list is generated by Yocto during the build and was not available when this repository was assembled. It belongs in `artifacts/licenses/license.manifest` (see [`artifacts/README.md`](artifacts/README.md)). Do not treat the table below as that list. It is a summary of what is expected, from the components used, and should be checked against the generated manifest.

| Component | Expected licence | Note |
|---|---|---|
| Linux kernel and NVIDIA open kernel modules | GPL-2.0 | Includes the `rtl8822ce` driver (GPL-2.0) |
| BusyBox, NetworkManager, e2fsprogs | GPL-2.0 (some LGPL parts) | |
| systemd, glibc | LGPL-2.1+ | |
| OpenSSH | BSD-style | |
| ROS 2 Humble packages, OpenCV 4.9 | Apache-2.0 | |
| CUDA, cuDNN, TensorRT, L4T userspace libraries | NVIDIA proprietary licences | Redistribution is restricted, which is why the flash image is not published here |
| `tegra-firmware-rtl8822` | Realtek firmware licence | Redistributable as firmware, not open source |
| **`yolov8n.onnx`** (model weights) | **AGPL-3.0** (Ultralytics) | Check this before any commercial use or distribution. Under the AGPL, offering the model over a network can carry source-sharing duties. |
| `meta-myrobot` (this repo) | MIT | `meta-myrobot/COPYING.MIT` |
| Yocto layers (poky, meta-openembedded) | MIT, with per-recipe licences | |

Generate the real list on the workstation after a build:

```bash
cat ~/yocto/build/tmp/deploy/licenses/myrobot-image-p3737-0000-p3701-0005*/license.manifest > artifacts/licenses/license.manifest
# or use ./scripts/collect-artifacts.sh
```

To audit only the non-open licences: `grep -B1 -A3 -i -E "proprietary|nvidia|CLOSED" license.manifest`.

## 7. Limitations and future work

| Item | Detail |
|---|---|
| Hard real-time | Build with `PREEMPT_RT`, then measure with `cyclictest` under load |
| Read-only root | Set `MYROBOT_RO_ROOT = "1"` and test. It also needs a writable `/var` and overlays. |
| Developer image | Build `myrobot-image-dev` and test `colcon build` on the board |
| Rebuild and re-flash with the networkd and `network-online` fixes | Not yet done. The fixes were verified only on the live system. |
| Kernel trim | Not done. The config is NVIDIA's default. |
| Application robustness | The node does not reopen the camera after an unplug. |
| Wi-Fi driver log noise | The `rtl8822ce` out-of-tree driver floods the console. |
| ROS 2 security | No SROS 2. |
| Updates | No A/B root update path. Only the boot chain has A/B slots. |
| Missing artifacts | Kernel `.config`, DTBs, the full package and licence manifests and the real `local.conf` still need to be exported from the workstation. |
