require myrobot-image.bb
SUMMARY = "MyRobot dev image (compilers, colcon, git, CUDA toolkit)"
IMAGE_FEATURES += "tools-sdk"
IMAGE_INSTALL:append = " packagegroup-ros2-dev cuda-toolkit demo-nodes-cpp"
