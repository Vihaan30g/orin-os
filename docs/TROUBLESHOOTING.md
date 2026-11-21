# Problems hit and how they were fixed

A log of what actually went wrong. Anyone repeating this will meet most of these.

## Build and flash

| Problem | Cause | Fix |
|---|---|---|
| First flash refused or the GPU failed (`nvgpu` CMA / comptags errors) | The image was built for the devkit machine definition `jetson-agx-orin-devkit` (SKU 0000), but the module is SKU 0005 (64 GB). meta-tegra issue #1613 covers the CMA side. | Temporary `TEGRA_BOARDSKU` workaround, then the real fix: `MACHINE = "p3737-0000-p3701-0005"`. CMA size was raised from 256 MB to 512 MB. |
| `libnvinfer_plugin.so.10` missing when running `trtexec` | The TensorRT plugin library was not in the image | Add `tensorrt-plugins` |
| CMake could not find CUDA while building `myrobot-yolo` | OpenCV is CUDA-enabled and pulls in its CUDA requirement | `inherit cuda` in the recipe |
| `bitbake` fetch failures | Unstable network on the workstation | Switch to a stable connection and rerun, since downloads resume |
| Layout override ignored; the build read meta-tegra's XML | `${THISDIR}` was expanded lazily inside meta-tegra | `MYROBOT_LAYOUT_DIR := "${THISDIR}/files"` (immediate expansion) |
| The 2.4 GB flash bundle was extracted into the build directory | `mkdir flash-8a && cd flash-8a` failed because the folder existed, so `tar` ran in the wrong place | Cleaned up with a guarded list-then-delete. `scripts/flash.sh` now refuses to reuse an existing folder. |
| `DATA` partition held stale files from the old root filesystem | The new partition overlapped old filesystem data on the eMMC | Wiped the first 16 MB of the partition with `dd`, then let `x-systemd.makefs` format it |
| `git clone` of poky over `git://` failed | Cause not investigated (the `git://` protocol did not work from the workstation) | Use `https://` |
| Multi-line commands in the PDF guide broke when pasted | Line wrapping in the PDF | Use single-line commands joined with `&&` |

## First boot and services

| Problem | Cause | Fix |
|---|---|---|
| `myrobot-datadirs.service` failed: `install: command not found` | BusyBox has no `install` | Replaced with `mkdir -p`, `chown` and `chmod` |
| SSH key login not working although the key was installed | Our `AuthorizedKeysFile` override was ignored. sshd takes the first such line it finds. | `myrobot_sshfix` deletes any existing line and then adds ours |
| `Host key verification failed` after every reflash | A reflash generates new host keys | `ssh-keygen -R <ip>` before connecting again |
| `systemctl stop myrobot-yolo` marked the unit `failed` | The node returned exit code 1 on shutdown | `rc = rclcpp::ok() ? 1 : 0;` in `yolo_node.cpp` |
| `/home/root` does not exist, so `scp` to it failed | Root's home is `/root` on this image | Use the right path, or `ssh ... cat` to copy files |
| `picocom` said `cannot lock /dev/ttyACM0` | A stale `picocom` process held the port | Kill the old process, then reconnect |

## Boot time

The image took **2 min 11 s** to boot (kernel 10.0 s, userspace 2 min 1 s; screenshot [`21`](images/21-boot-time-before.png)).

- **Cause.** `systemd-networkd-wait-online.service` waits up to 120 s for a network that NetworkManager, not networkd, manages.
- **Fix applied live.** Masked `systemd-networkd.service`, `systemd-networkd.socket` and `systemd-networkd-wait-online.service` on the board (screenshot [`22`](images/22-boot-fix-live.png)). The next boot took **21.284 s** (kernel 10.8 s, userspace 10.5 s; screenshot [`23`](images/23-boot-time-after.png)).
- **Status in the repo.** The masks are now in `myrobot_mask`, but no image has been rebuilt and flashed with them.

## Networking and the demo

| Problem | Cause | Fix |
|---|---|---|
| The laptop could not see `/detections` after boot | The detector started before Wi-Fi had an address, so the node never advertised on the Wi-Fi interface | Restarting the service fixed it. `network-online.target` was added to the unit's `After=` and `Wants=` (applied live, and now in the repo). |
| Laptop saw no topics at all | The laptop's `.bashrc` forced `ROS_DOMAIN_ID=63` and CycloneDDS | The demo terminal overrides it to `ROS_DOMAIN_ID=42` (the same as the Orin) and `RMW_IMPLEMENTATION=rmw_fastrtps_cpp`. CycloneDDS also worked. |
| Serial console flooded with `txbd` / `desc` dumps | The out-of-tree `rtl8822ce` Wi-Fi driver logs at a high level | `dmesg -n 1`. Not fixed in the image. |
| `nmcli`: "Secrets were required" | A stale connection profile | Delete the profile and reconnect with the SSID and password quoted |
| Camera numbering changed (`video1`/`video2`) after unplugging | The node holds `/dev/video0` open while it is unplugged, so the replug gets a new number | Stop the service, replug, start the service. The node does not reopen the device (a known limitation). |

## Mistakes that cost time

- Running a command on the wrong machine (workstation, Orin or laptop), including `sudo ssh`, which uses root's keys and not yours. The demo steps in [`TESTS.md`](TESTS.md) name the machine for each command.
- Not checking the board's SKU before the first build. Check `CHECK_BOARDSKU` in the flash output.
