require myrobot-image.bb
SUMMARY = "MyRobot dev image (adds compilers and git)"
IMAGE_FEATURES += "tools-sdk"
IMAGE_INSTALL:append = " git cmake"
