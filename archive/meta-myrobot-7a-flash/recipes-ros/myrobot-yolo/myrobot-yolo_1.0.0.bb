DESCRIPTION = "YOLOv8 TensorRT webcam detector node for ROS 2 Humble"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit ros_distro_humble
inherit systemd
inherit cuda

SRC_URI = " \
    file://myrobot_yolo/CMakeLists.txt \
    file://myrobot_yolo/package.xml \
    file://myrobot_yolo/src/yolo_node.cpp \
    file://myrobot-yolo.service \
    file://myrobot-datadirs.service \
    file://yolov8n.onnx \
"
S = "${WORKDIR}/myrobot_yolo"

ROS_CN = "myrobot"
ROS_BPN = "myrobot_yolo"
ROS_BUILD_DEPENDS = "rclcpp vision-msgs"
ROS_BUILDTOOL_DEPENDS = "ament-cmake-native"
ROS_EXEC_DEPENDS = "rclcpp vision-msgs"
ROS_BUILD_TYPE = "ament_cmake"

DEPENDS = "${ROS_BUILD_DEPENDS} ${ROS_BUILDTOOL_DEPENDS} tensorrt-core cuda-cudart opencv"
RDEPENDS:${PN} += "${ROS_EXEC_DEPENDS} tensorrt-core tensorrt-plugins tensorrt-trtexec cuda-libraries opencv"

inherit ros_${ROS_BUILD_TYPE}

SYSTEMD_SERVICE:${PN} = "myrobot-yolo.service myrobot-datadirs.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/myrobot-yolo.service ${D}${systemd_system_unitdir}/myrobot-yolo.service
    install -m 0644 ${WORKDIR}/myrobot-datadirs.service ${D}${systemd_system_unitdir}/myrobot-datadirs.service
    install -d ${D}${datadir}/myrobot
    install -m 0644 ${WORKDIR}/yolov8n.onnx ${D}${datadir}/myrobot/yolov8n.onnx
}
FILES:${PN} += "${systemd_system_unitdir} ${datadir}/myrobot"
