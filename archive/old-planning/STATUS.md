# Project Status

**Last updated:** 2026-10-21 (Week 1, kickoff)
**Overall health:** Green (just started)
**Current phase:** Phase 0, planning and baseline

## Milestones

| # | Milestone | Target | State |
|---|---|---|---|
| 0 | Kickoff, scope and plan agreed | Week 1 | Done |
| 1 | JetPack baseline measured (size, idle RAM, boot time, YOLO FPS) | Week 1-2 | Not started |
| 2 | Yocto host set up, `meta-tegra` image boots on AGX Orin | TBD | Not started |
| 3 | Custom kernel defconfig, trimmed drivers | TBD | Not started |
| 4 | Read-only rootfs and ROS 2 (`meta-ros`) in image | TBD | Not started |
| 5 | Boot-time ROS 2 launch service | TBD | Not started |
| 6 | ZED 2i + CUDA/TensorRT working, ESP32 link verified | TBD | Not started |
| 7 | Benchmark orin-os vs JetPack, final report | TBD | Not started |

## Done

- Project scope and objectives written up in [README.md](README.md).
- Repo, status file and engineering log created.

## In progress

- Nothing yet.

## Next

- Record JetPack baseline numbers on the target Orin (image size, idle RAM, boot-to-first-topic, YOLO FPS on ZED 2i).
- Assign owners to each milestone.
- Set up the Yocto build host and check the `meta-tegra` release matches our L4T/JetPack version.

## Risks and blockers

| Risk | Impact | Mitigation |
|---|---|---|
| `meta-tegra` / L4T version support for AGX Orin and ZED SDK | Camera may not work on the custom image | Check compatibility early, before kernel trimming |
| ZED SDK and CUDA/TensorRT packaging in Yocto | Hardware acceleration milestone slips | Prototype the recipes in Milestone 2 |
| Over-aggressive kernel trimming | Missing driver found late | Trim in steps, keep a known-good config |
| Read-only rootfs vs apps that need to write (logs, ROS cache) | Runtime failures | Plan tmpfs or a writable data partition |
| Yocto build time and disk needs | Slow iteration | Use a strong build host, shared sstate cache |

## Open questions

- Which JetPack/L4T version is the baseline?
- systemd (trimmed) or sysvinit/BusyBox init?
- Bluetooth gamepad needed for manual override?
- micro-ROS or custom serial packets for the ESP32 link?
- OTA update tool (Mender or other)?
