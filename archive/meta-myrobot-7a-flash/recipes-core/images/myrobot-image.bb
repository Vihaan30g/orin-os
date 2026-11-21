SUMMARY = "MyRobot production image"
LICENSE = "MIT"
inherit core-image extrausers

# No debug-tweaks: SSH is key-only. Root keeps an empty password on the SERIAL console only.
EXTRA_IMAGE_FEATURES = ""
IMAGE_FEATURES += "ssh-server-openssh empty-root-password"
MYROBOT_RO_ROOT ?= "0"
IMAGE_FEATURES += "${@'read-only-rootfs' if d.getVar('MYROBOT_RO_ROOT') == '1' else ''}"

IMAGE_INSTALL:append = " openssh v4l-utils networkmanager wpa-supplicant iproute2 sudo e2fsprogs-mke2fs"
IMAGE_INSTALL:append = " kernel-module-uvcvideo kernel-module-cfg80211 kernel-module-mac80211 kernel-module-rtl8822ce tegra-firmware-rtl8822"
IMAGE_INSTALL:append = " cuda-libraries cudnn tensorrt-core tensorrt-plugins tensorrt-trtexec tegra-nvpmodel tegra-tools-tegrastats systemd-analyze"
IMAGE_INSTALL:append = " ros-core std-msgs sensor-msgs vision-msgs opencv"

EXTRA_USERS_PARAMS = "useradd -u 1000 -U -m -s /bin/sh -G video rover; usermod -p '*' rover;"

ROOTFS_POSTPROCESS_COMMAND += "myrobot_config;"
myrobot_config() {
    # SSH: key only; keys live in the (read-only) /etc
    install -d -m 0755 ${IMAGE_ROOTFS}${sysconfdir}/ssh/authorized_keys
    echo "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIH4jpG3gSNxnSKvvOUwZ8h5HRaLD/7/8IS8yTrT9TMmT marsuser@marsnew" > ${IMAGE_ROOTFS}${sysconfdir}/ssh/authorized_keys/rover
    chmod 0644 ${IMAGE_ROOTFS}${sysconfdir}/ssh/authorized_keys/rover
    for f in sshd_config sshd_config_readonly; do
        if [ -f ${IMAGE_ROOTFS}${sysconfdir}/ssh/$f ]; then
            printf '%s\n' 'AuthorizedKeysFile /etc/ssh/authorized_keys/%u' 'PermitRootLogin no' 'PasswordAuthentication no' 'PermitEmptyPasswords no' 'KbdInteractiveAuthentication no' >> ${IMAGE_ROOTFS}${sysconfdir}/ssh/$f
        fi
    done

    # DATA partition is the writable /home/rover
    install -d ${IMAGE_ROOTFS}/home/rover
    echo 'PARTLABEL=DATA /home/rover ext4 defaults,noatime,nofail,x-systemd.makefs 0 2' >> ${IMAGE_ROOTFS}${sysconfdir}/fstab
    install -d ${IMAGE_ROOTFS}${sysconfdir}/tmpfiles.d
    printf '%s\n' 'd /home/rover 0755 rover rover -' 'd /home/rover/ros2_ws 0755 rover rover -' > ${IMAGE_ROOTFS}${sysconfdir}/tmpfiles.d/myrobot.conf

    # passwordless sudo for the owner's account, ROS environment
    install -d -m 0750 ${IMAGE_ROOTFS}${sysconfdir}/sudoers.d
    echo 'rover ALL=(ALL) NOPASSWD: ALL' > ${IMAGE_ROOTFS}${sysconfdir}/sudoers.d/rover
    chmod 0440 ${IMAGE_ROOTFS}${sysconfdir}/sudoers.d/rover
    printf '%s\n' '[ -f /opt/ros/humble/setup.sh ] && . /opt/ros/humble/setup.sh' 'export ROS_LOG_DIR=/home/rover/ros2_ws/log' 'export ROS_DOMAIN_ID=42' > ${IMAGE_ROOTFS}${sysconfdir}/profile.d/myrobot.sh
}

ROOTFS_POSTPROCESS_COMMAND += "myrobot_sshfix;"
myrobot_sshfix() {
    for f in sshd_config sshd_config_readonly; do
        if [ -f ${IMAGE_ROOTFS}${sysconfdir}/ssh/$f ]; then
            sed -i '/^[[:space:]]*AuthorizedKeysFile/d' ${IMAGE_ROOTFS}${sysconfdir}/ssh/$f
            echo 'AuthorizedKeysFile /etc/ssh/authorized_keys/%u' >> ${IMAGE_ROOTFS}${sysconfdir}/ssh/$f
        fi
    done
}
IMAGE_INSTALL:append = " myrobot-yolo"
