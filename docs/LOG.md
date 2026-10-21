# Engineering Log

Append-only. Newest entry at the bottom. One entry per working session or meeting.

---

## 2026-10-21, Week 1: Kickoff meeting

**Attendees:** TBD

**Discussed**
- Project scope: custom Yocto-based OS for the Jetson AGX Orin + ESP32 rover, replacing JetPack (see [README](../README.md)).
- Objectives: at least 60% smaller image, under 10 s boot, native L4T/CUDA/TensorRT, low-latency ESP32 link.
- Approach: Yocto with `meta-tegra` and `meta-ros`, custom kernel defconfig, read-only rootfs, boot-time ROS 2 service.
- What to strip from the stock image (GUI, extra services, dev tools, unused drivers, APT).
- Evaluation: image size, idle RAM, boot-to-first-topic, YOLO FPS on ZED 2i, all against stock JetPack.

**Decisions**
- Keep the project docs in this repo: README (definition), STATUS.md (current state), this log (history).
- Hold a weekly meeting with a short slide deck built from STATUS.md and this log.
- Other decisions: TBD

**Action items**

| Action | Owner | Due |
|---|---|---|
| Measure JetPack baseline metrics | TBD | TBD |
| Set up Yocto build host | TBD | TBD |
| Confirm `meta-tegra` release for our L4T version | TBD | TBD |
