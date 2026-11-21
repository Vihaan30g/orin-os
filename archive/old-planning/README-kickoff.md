# orin-os

A lean, application-specific Linux distribution for a heterogeneous autonomous rover, built with Yocto to replace stock NVIDIA JetPack/Ubuntu.

- **Status:** see [STATUS.md](STATUS.md)
- **Engineering log:** see [docs/LOG.md](docs/LOG.md)
- **Weekly reporting:** see [docs/weekly/README.md](docs/weekly/README.md)

## 1. Overview

The rover has two compute targets:

| Target | Role |
|---|---|
| NVIDIA Jetson AGX Orin | Heavy compute: spatial AI, autonomous navigation (ROS 2, CUDA, RTAB-Map) |
| ESP32 microcontroller | Real-time actuator control: 4-wheel drive and a 6-DOF manipulator arm |

The stock JetPack/Ubuntu image is general-purpose and heavy. This project builds a custom OS that boots faster, uses less RAM, and leaves the maximum resources for the ROS 2 and CUDA workloads.

## 2. Objectives

1. **Minimal footprint:** remove GNOME, unneeded services and unused drivers. Target is an image at least 60% smaller than stock JetPack.
2. **Rapid boot:** target is under 10 s from power-on to the application running, so the rover recovers quickly in the field.
3. **Hardware acceleration:** natively integrate NVIDIA Linux for Tegra (L4T) drivers, the CUDA runtime and TensorRT, so the ZED 2i and webcams can run high-framerate detection and spatial mapping (RTAB-Map).
4. **Seamless MCU integration:** pre-configure kernel modules and USB serial drivers for low-latency Orin to ESP32 communication (micro-ROS or custom serial packets).

## 3. Build methodology

- **Build system:** Yocto Project with the `meta-tegra` and `meta-ros` layers.
- **Kernel:** custom defconfig that compiles only the needed drivers (USB, I2C, SPI, UART, networking for the Ubiquiti radio).
- **Root filesystem:** read-only, to survive sudden power loss.
- **Application start:** a custom system service launches the ROS 2 launch files at boot.

## 4. What gets stripped

| Area | Removed |
|---|---|
| GUI | GNOME (`ubuntu-desktop`), `gdm3`, X11, Wayland, desktop apps (LibreOffice, Chromium, editors, image viewers). Boot target `multi-user.target` instead of `graphical.target` (frees roughly 1.6 GB RAM, per our initial research; to be verified in the baseline). |
| Services | ModemManager, NetworkManager (replaced by static IP and a minimal `wpa_supplicant`), Bluetooth/BlueZ (unless a gamepad is used), CUPS, unattended-upgrades. Evaluate sysvinit/BusyBox init versus a trimmed systemd. |
| Dev tools | GCC/G++ and `nvcc` plus CUDA headers. Keep the CUDA runtime libraries. Cross-compile on the host through Yocto. |
| Kernel drivers | ALSA/PulseAudio, generic USB/PCIe device drivers, unused filesystems (BTRFS, XFS, NTFS). Keep drivers for ZED 2i, ESP32 serial and the radio only. |
| Package manager | APT/dpkg and caches. The image is immutable. Updates go through an OTA tool (for example Mender). |

## 5. Evaluation and benchmarking

Compared directly against stock JetPack on the same hardware.

| Metric | JetPack baseline | orin-os | Target |
|---|---|---|---|
| Total OS image size (MB) | TBD | TBD | at least 60% smaller |
| Idle RAM before ROS 2 stack (MB) | TBD | TBD | as low as possible |
| Boot time, power-on to first ROS 2 topic (s) | TBD | TBD | under 10 s |
| YOLO inference FPS on ZED 2i stream | TBD | TBD | higher than baseline |

## 6. Repository layout

```
README.md          project definition (this file)
STATUS.md          current state, milestones, risks
docs/LOG.md        dated, append-only engineering log
docs/weekly/       weekly meeting decks and cadence notes
```

Yocto layers, kernel config and service files will be added as the build is set up.
