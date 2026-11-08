SUMMARY = "MyRobot production image"
LICENSE = "MIT"
inherit core-image
IMAGE_FEATURES += "ssh-server-openssh"
IMAGE_INSTALL:append = " openssh v4l-utils networkmanager wpa-supplicant iproute2 kernel-module-uvcvideo kernel-module-cfg80211 kernel-module-mac80211 kernel-module-rtl8822ce tegra-firmware-rtl8822"
IMAGE_INSTALL:append = " cuda-libraries cudnn tensorrt-core tensorrt-trtexec tegra-nvpmodel systemd-analyze"
