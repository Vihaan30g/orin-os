cmake_minimum_required(VERSION 3.8)
project(myrobot_yolo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio)

find_path(TRT_INCLUDE_DIR NvInfer.h)
find_library(NVINFER_LIB NAMES nvinfer libnvinfer.so.10)
find_path(CUDA_INCLUDE_DIR cuda_runtime_api.h
          HINTS /usr/local/cuda-12.6/include /usr/local/cuda/include)
find_library(CUDART_LIB NAMES cudart libcudart.so.12
             HINTS /usr/local/cuda-12.6/lib /usr/local/cuda-12.6/lib64 /usr/local/cuda/lib64)

if(NOT TRT_INCLUDE_DIR OR NOT NVINFER_LIB OR NOT CUDA_INCLUDE_DIR OR NOT CUDART_LIB)
  message(FATAL_ERROR "TensorRT/CUDA not found: ${TRT_INCLUDE_DIR} ${NVINFER_LIB} ${CUDA_INCLUDE_DIR} ${CUDART_LIB}")
endif()

add_executable(yolo_node src/yolo_node.cpp)
ament_target_dependencies(yolo_node rclcpp vision_msgs)
target_include_directories(yolo_node PRIVATE ${TRT_INCLUDE_DIR} ${CUDA_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(yolo_node ${NVINFER_LIB} ${CUDART_LIB} ${OpenCV_LIBS})

install(TARGETS yolo_node DESTINATION lib/${PROJECT_NAME})
ament_package()