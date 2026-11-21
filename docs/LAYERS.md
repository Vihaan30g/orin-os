# Layers and exactly what they contain

The image is assembled from five layers. Four are upstream and pinned by commit. One, `meta-myrobot`, is mine.

## Upstream layers (pinned, not modified)

| Layer | Pinned commit | Role in this image |
|---|---|---|
| `poky` (OpenEmbedded-Core, scarthgap, 5.0.21) | `9517df0a9af21f1e0f9d6ba5a78c5ae1d7c83184` | BitBake, base recipes, the `poky` distro, systemd, BusyBox, `core-image` classes |
| `meta-openembedded` | `2db6aece32c7f8ebf6015b468e1b4e21de072e3b` | NetworkManager, wpa-supplicant, v4l-utils and the libraries that ROS 2 builds on |
| `meta-tegra` (OE4T, L4T R36.5.2) | `6d7cc8f27e8b3bda2abfa68b2fcfe8d6ecf10464` | Jetson support: the machine definition `p3737-0000-p3701-0005`, bootloader, Linux `5.15.199-l4t-r36.5-1050.50`, NVIDIA out-of-tree modules (`nvgpu` and others), CUDA 12.6, cuDNN 9.3, TensorRT 10.3, `tegra-nvpmodel`, `tegra-tools-tegrastats`, the `initrd-flash` tooling and the stock partition layout |
| `meta-ros` (ROS 2 Humble) | `d8b213ec652c7f5ec0fb725cde2d041aaaec3ce5` | `ros-core`, `std-msgs`, `sensor-msgs`, `vision-msgs`, the ament build classes and the ROS 2 dependencies of the detector |

Source: [`evidence/layer-versions.txt`](../evidence/layer-versions.txt), shown in screenshot [`19`](images/19-manifest-layer-versions.png).

## `meta-myrobot` (this repository)

`conf/layer.conf`: collection name `meta-myrobot`, priority 6, depends on `core`, compatible with `scarthgap`.

```
meta-myrobot/
├── conf/layer.conf
├── recipes-core/images/
│   ├── myrobot-image.bb          the production image
│   └── myrobot-image-dev.bb      developer variant (written, NEVER BUILT)
├── recipes-bsp/tegra-binaries/
│   ├── tegra-storage-layout-base_%.bbappend   points meta-tegra at our partition XML
│   └── files/flash_t234_qspi_sdmmc.xml        NVIDIA's layout plus the DATA partition
├── recipes-ros/myrobot-yolo/
│   ├── myrobot-yolo_1.0.0.bb     the detector ROS 2 package recipe
│   └── files/                    node source, two systemd units, yolov8n.onnx
└── recipes-example/example/      create-layer boilerplate, unused
```

### `myrobot-image.bb`: what is in the image

| Group | Packages and settings |
|---|---|
| Image base | `inherit core-image extrausers`. `EXTRA_IMAGE_FEATURES` is empty. |
| Features | `ssh-server-openssh`, `empty-root-password`. Root has an empty password on the serial console only. SSH root login and password login are disabled. |
| Access and network | `openssh`, `networkmanager`, `wpa-supplicant`, `iproute2`, `iproute2-ss`, `sudo` |
| Storage | `e2fsprogs-mke2fs`, which is used by `x-systemd.makefs` to format `DATA` on first boot |
| Camera and Wi-Fi | `v4l-utils`, `kernel-module-uvcvideo`, `kernel-module-cfg80211`, `kernel-module-mac80211`, `kernel-module-rtl8822ce`, `tegra-firmware-rtl8822` |
| GPU stack | `cuda-libraries`, `cudnn`, `tensorrt-core`, `tensorrt-plugins`, `tensorrt-trtexec` |
| Jetson tools | `tegra-nvpmodel`, `tegra-tools-tegrastats`, `systemd-analyze` |
| ROS 2 | `ros-core`, `std-msgs`, `sensor-msgs`, `vision-msgs`, `opencv` |
| Application | `myrobot-yolo` |
| User | `rover` (uid 1000, group `video`, shell `/bin/sh`, no password, `sudo` without a password) |

Post-processing functions that run on the root filesystem:

| Function | Effect |
|---|---|
| `myrobot_config` | Writes the SSH public key to `/etc/ssh/authorized_keys/rover`. Sets `PermitRootLogin no`, `PasswordAuthentication no` and `PermitEmptyPasswords no`. Adds the fstab line `PARTLABEL=DATA /home/rover ext4 defaults,noatime,nofail,x-systemd.makefs 0 2`. Adds a tmpfiles rule, a sudoers entry for `rover` and `/etc/profile.d/myrobot.sh` (`ROS_DOMAIN_ID=42`, `ROS_LOG_DIR`). |
| `myrobot_sshfix` | Removes any existing `AuthorizedKeysFile` line and adds ours, because sshd uses the first one it finds. |
| `myrobot_mask` | Masks `avahi-daemon`, `bluetooth`, `ofono`, `rpcbind` and `dnsmasq`. In the current source it also masks `systemd-networkd` and `systemd-networkd-wait-online` (see the note below). |

`MYROBOT_RO_ROOT ?= "0"` toggles the `read-only-rootfs` feature. It is off, and it has not been tested.

**Note on the networkd mask.** The masks for `systemd-networkd.service`, `systemd-networkd.socket` and `systemd-networkd-wait-online.service` were first applied by hand on the running board. That cut boot from 2 min 11 s to 21.3 s. They were added to `myrobot_mask` afterwards when this repository was assembled, so **the 8b image that was flashed does not contain them**, and the source has not been rebuilt and tested since.

### `myrobot-image-dev.bb`

`require myrobot-image.bb`, plus `tools-sdk`, `packagegroup-ros2-dev`, `cuda-toolkit` and `demo-nodes-cpp`. It is meant to let you run `colcon build` on the board. It was **not built**.

### `tegra-storage-layout-base_%.bbappend` and `flash_t234_qspi_sdmmc.xml`

The bbappend sets `PARTITION_FILE` to our copy of the XML. It uses `:=` for the layer directory, so the path is expanded in this layer and not in meta-tegra's folder (this was a bug, see [`TROUBLESHOOTING.md`](TROUBLESHOOTING.md)). The XML is NVIDIA's `flash_t234_qspi_sdmmc.xml` with one added partition:

| Partition | Size | Notes |
|---|---|---|
| `DATA` | 34,359,738,368 bytes (32 GiB) | Added before `APP`. `allocation_attribute 8`, align 16384. Becomes `mmcblk0p16`. |
| `APP` (rootfs) | fills the rest | Gets `myrobot-image.ext4` (16 GiB image into a 25.8 GiB partition) |

### `myrobot-yolo_1.0.0.bb` and its files

- Inherits `ros_distro_humble`, `systemd`, `cuda` and `ros_${ROS_BUILD_TYPE}`. `inherit cuda` is needed because OpenCV is CUDA-enabled and CMake could not find CUDA otherwise.
- `DEPENDS`: `tensorrt-core`, `cuda-cudart`, `opencv` and the ROS message packages.
- Installs `yolov8n.onnx` (12.8 MB) to `/usr/share/myrobot/`, and two services.
- **`myrobot_yolo/src/yolo_node.cpp`**: opens the webcam through OpenCV V4L2 (MJPG, 640x480), letterboxes to 640x640, runs TensorRT 10 `enqueueV3`, decodes the YOLOv8 output `[1,84,8400]`, applies NMS and publishes `vision_msgs/Detection2DArray` on `detections`. It logs the FPS every 5 s.
- **`myrobot-yolo.service`**: runs as `rover`. `ExecStartPre` builds the TensorRT FP16 engine with `trtexec` if `/home/rover/models/yolov8n.engine` is missing. `ExecStart` runs `yolo_node` with that engine. It uses `Restart=on-failure`, and is ordered after `network-online.target` and NetworkManager.
- **`myrobot-datadirs.service`**: creates `/home/rover/models`, `/home/rover/ros2_ws` and the log directory, and sets ownership. It uses `mkdir`, `chown` and `chmod` and not `install`, because BusyBox has no `install` (an earlier bug).

## `archive/meta-myrobot-7a-flash/`

The older copy of the layer, as flashed in run 7a. It differs from the current one: it has the `install -d` datadirs service, no `myrobot_mask`, no `iproute2-ss`, and a different node exit code. It is kept for history only.
