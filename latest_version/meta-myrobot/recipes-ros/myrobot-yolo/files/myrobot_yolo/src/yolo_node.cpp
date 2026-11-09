// myrobot_yolo: webcam -> TensorRT (YOLOv8 ONNX engine) -> vision_msgs/Detection2DArray
#include <NvInfer.h>
#include <cuda_runtime_api.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "rclcpp/rclcpp.hpp"
#include "vision_msgs/msg/detection2_d_array.hpp"

namespace {

const char* kNames[80] = {
  "person","bicycle","car","motorcycle","airplane","bus","train","truck","boat","traffic light",
  "fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow",
  "elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee",
  "skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard",
  "tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple",
  "sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","couch",
  "potted plant","bed","dining table","toilet","tv","laptop","mouse","remote","keyboard","cell phone",
  "microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear",
  "hair drier","toothbrush"};

#define CUDA_CHECK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) \
  throw std::runtime_error(std::string("CUDA error: ") + cudaGetErrorString(e_)); } while (0)

class TrtLogger : public nvinfer1::ILogger {
 public:
  void log(Severity severity, const char* msg) noexcept override {
    if (severity <= Severity::kWARNING) {
      RCLCPP_WARN(rclcpp::get_logger("tensorrt"), "%s", msg);
    }
  }
};

struct Det { float x1, y1, x2, y2, score; int cls; };

float Iou(const Det& a, const Det& b) {
  float ix1 = std::max(a.x1, b.x1), iy1 = std::max(a.y1, b.y1);
  float ix2 = std::min(a.x2, b.x2), iy2 = std::min(a.y2, b.y2);
  float iw = std::max(0.f, ix2 - ix1), ih = std::max(0.f, iy2 - iy1);
  float inter = iw * ih;
  float ua = (a.x2 - a.x1) * (a.y2 - a.y1) + (b.x2 - b.x1) * (b.y2 - b.y1) - inter;
  return ua > 0.f ? inter / ua : 0.f;
}

std::vector<Det> Nms(std::vector<Det> d, float thr) {
  std::sort(d.begin(), d.end(), [](const Det& a, const Det& b) { return a.score > b.score; });
  std::vector<Det> keep;
  for (const auto& c : d) {
    bool ok = true;
    for (const auto& k : keep) {
      if (k.cls == c.cls && Iou(k, c) > thr) { ok = false; break; }
    }
    if (ok) keep.push_back(c);
  }
  return keep;
}

// vision_msgs 4.x layouts differ slightly between releases; pick whichever compiles.
template <typename C>
auto SetCenter(C& c, double x, double y, int) -> decltype((void)(c.position.x = x), void()) {
  c.position.x = x; c.position.y = y;
}
template <typename C>
auto SetCenter(C& c, double x, double y, long) -> decltype((void)(c.x = x), void()) {
  c.x = x; c.y = y;
}

}  // namespace

class YoloNode : public rclcpp::Node {
 public:
  YoloNode() : Node("yolo_node") {
    engine_path_ = declare_parameter<std::string>("engine_path", "/home/rover/models/yolov8n.engine");
    device_ = declare_parameter<std::string>("camera_device", "/dev/video0");
    cam_w_ = declare_parameter<int>("width", 640);
    cam_h_ = declare_parameter<int>("height", 480);
    cam_fps_ = declare_parameter<int>("fps", 30);
    conf_thr_ = static_cast<float>(declare_parameter<double>("conf_threshold", 0.25));
    nms_thr_ = static_cast<float>(declare_parameter<double>("nms_threshold", 0.45));
    pub_ = create_publisher<vision_msgs::msg::Detection2DArray>("detections", 10);

    LoadEngine();
    OpenCamera();
    last_report_ = std::chrono::steady_clock::now();
  }

  ~YoloNode() override {
    if (d_in_) cudaFree(d_in_);
    if (d_out_) cudaFree(d_out_);
    if (stream_) cudaStreamDestroy(stream_);
  }

  bool Step() {
    cv::Mat frame;
    if (!cap_.read(frame) || frame.empty()) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "camera read failed");
      return false;
    }
    auto t0 = std::chrono::steady_clock::now();

    // Letterbox to the network input size.
    float scale = std::min(in_w_ / static_cast<float>(frame.cols), in_h_ / static_cast<float>(frame.rows));
    int nw = static_cast<int>(std::round(frame.cols * scale));
    int nh = static_cast<int>(std::round(frame.rows * scale));
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(nw, nh));
    cv::Mat canvas(in_h_, in_w_, CV_8UC3, cv::Scalar(114, 114, 114));
    int pad_x = (in_w_ - nw) / 2, pad_y = (in_h_ - nh) / 2;
    resized.copyTo(canvas(cv::Rect(pad_x, pad_y, nw, nh)));

    // BGR uint8 HWC -> RGB float CHW, 0..1
    const int plane = in_h_ * in_w_;
    for (int y = 0; y < in_h_; ++y) {
      const uint8_t* row = canvas.ptr<uint8_t>(y);
      for (int x = 0; x < in_w_; ++x) {
        const int i = y * in_w_ + x;
        host_in_[i] = row[3 * x + 2] / 255.f;
        host_in_[plane + i] = row[3 * x + 1] / 255.f;
        host_in_[2 * plane + i] = row[3 * x] / 255.f;
      }
    }

    CUDA_CHECK(cudaMemcpyAsync(d_in_, host_in_.data(), host_in_.size() * sizeof(float), cudaMemcpyHostToDevice, stream_));
    if (!context_->enqueueV3(stream_)) throw std::runtime_error("enqueueV3 failed");
    CUDA_CHECK(cudaMemcpyAsync(host_out_.data(), d_out_, host_out_.size() * sizeof(float), cudaMemcpyDeviceToHost, stream_));
    CUDA_CHECK(cudaStreamSynchronize(stream_));

    // Decode YOLOv8 output [1, 4 + classes, boxes]
    std::vector<Det> dets;
    for (int i = 0; i < n_boxes_; ++i) {
      int best = 0; float best_s = 0.f;
      for (int c = 0; c < n_cls_; ++c) {
        float s = host_out_[(4 + c) * n_boxes_ + i];
        if (s > best_s) { best_s = s; best = c; }
      }
      if (best_s < conf_thr_) continue;
      float cx = host_out_[0 * n_boxes_ + i], cy = host_out_[1 * n_boxes_ + i];
      float w = host_out_[2 * n_boxes_ + i], h = host_out_[3 * n_boxes_ + i];
      Det d;
      d.x1 = std::clamp((cx - w / 2 - pad_x) / scale, 0.f, static_cast<float>(frame.cols));
      d.y1 = std::clamp((cy - h / 2 - pad_y) / scale, 0.f, static_cast<float>(frame.rows));
      d.x2 = std::clamp((cx + w / 2 - pad_x) / scale, 0.f, static_cast<float>(frame.cols));
      d.y2 = std::clamp((cy + h / 2 - pad_y) / scale, 0.f, static_cast<float>(frame.rows));
      d.score = best_s; d.cls = best;
      dets.push_back(d);
    }
    dets = Nms(dets, nms_thr_);

    vision_msgs::msg::Detection2DArray msg;
    msg.header.stamp = now();
    msg.header.frame_id = "camera";
    for (const auto& d : dets) {
      vision_msgs::msg::Detection2D det;
      det.header = msg.header;
      SetCenter(det.bbox.center, (d.x1 + d.x2) / 2.0, (d.y1 + d.y2) / 2.0, 0);
      det.bbox.size_x = d.x2 - d.x1;
      det.bbox.size_y = d.y2 - d.y1;
      vision_msgs::msg::ObjectHypothesisWithPose hyp;
      hyp.hypothesis.class_id = (d.cls >= 0 && d.cls < 80) ? kNames[d.cls] : std::to_string(d.cls);
      hyp.hypothesis.score = d.score;
      det.results.push_back(hyp);
      msg.detections.push_back(det);
    }
    pub_->publish(msg);

    // FPS report every 5 s
    ++frames_;
    auto t1 = std::chrono::steady_clock::now();
    infer_ms_ += std::chrono::duration<double, std::milli>(t1 - t0).count();
    double since = std::chrono::duration<double>(t1 - last_report_).count();
    if (since >= 5.0) {
      RCLCPP_INFO(get_logger(), "%.1f fps, %.1f ms/frame (capture excluded), %zu detections in last frame",
                  frames_ / since, infer_ms_ / frames_, dets.size());
      frames_ = 0; infer_ms_ = 0.0; last_report_ = t1;
    }
    return true;
  }

 private:
  void LoadEngine() {
    std::ifstream f(engine_path_, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open engine: " + engine_path_);
    std::vector<char> blob((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    runtime_.reset(nvinfer1::createInferRuntime(logger_));
    if (!runtime_) throw std::runtime_error("createInferRuntime failed");
    engine_.reset(runtime_->deserializeCudaEngine(blob.data(), blob.size()));
    if (!engine_) throw std::runtime_error("deserializeCudaEngine failed (engine built with a different TensorRT/GPU?)");
    context_.reset(engine_->createExecutionContext());
    if (!context_) throw std::runtime_error("createExecutionContext failed");

    std::string in_name, out_name;
    for (int i = 0; i < engine_->getNbIOTensors(); ++i) {
      const char* name = engine_->getIOTensorName(i);
      if (engine_->getTensorIOMode(name) == nvinfer1::TensorIOMode::kINPUT) in_name = name; else out_name = name;
    }
    auto id = engine_->getTensorShape(in_name.c_str());    // [1,3,H,W]
    auto od = engine_->getTensorShape(out_name.c_str());   // [1,4+classes,N]
    if (id.nbDims != 4 || od.nbDims != 3) throw std::runtime_error("unexpected tensor shapes");
    in_h_ = static_cast<int>(id.d[2]); in_w_ = static_cast<int>(id.d[3]);
    n_cls_ = static_cast<int>(od.d[1]) - 4; n_boxes_ = static_cast<int>(od.d[2]);
    if (in_h_ <= 0 || in_w_ <= 0 || n_cls_ <= 0 || n_boxes_ <= 0) throw std::runtime_error("dynamic shapes not supported");

    host_in_.assign(3 * in_h_ * in_w_, 0.f);
    host_out_.assign((4 + n_cls_) * n_boxes_, 0.f);
    CUDA_CHECK(cudaMalloc(&d_in_, host_in_.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out_, host_out_.size() * sizeof(float)));
    CUDA_CHECK(cudaStreamCreate(&stream_));
    context_->setTensorAddress(in_name.c_str(), d_in_);
    context_->setTensorAddress(out_name.c_str(), d_out_);
    RCLCPP_INFO(get_logger(), "engine loaded: input %dx%d, %d classes, %d boxes", in_w_, in_h_, n_cls_, n_boxes_);
  }

  void OpenCamera() {
    cap_.open(device_, cv::CAP_V4L2);
    if (!cap_.isOpened()) throw std::runtime_error("cannot open camera " + device_);
    cap_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, cam_w_);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, cam_h_);
    cap_.set(cv::CAP_PROP_FPS, cam_fps_);
    RCLCPP_INFO(get_logger(), "camera %s opened", device_.c_str());
  }

  std::string engine_path_, device_;
  int cam_w_{640}, cam_h_{480}, cam_fps_{30};
  float conf_thr_{0.25f}, nms_thr_{0.45f};
  rclcpp::Publisher<vision_msgs::msg::Detection2DArray>::SharedPtr pub_;
  TrtLogger logger_;
  std::unique_ptr<nvinfer1::IRuntime> runtime_;
  std::unique_ptr<nvinfer1::ICudaEngine> engine_;
  std::unique_ptr<nvinfer1::IExecutionContext> context_;
  cv::VideoCapture cap_;
  cudaStream_t stream_{nullptr};
  void* d_in_{nullptr};
  void* d_out_{nullptr};
  std::vector<float> host_in_, host_out_;
  int in_h_{0}, in_w_{0}, n_cls_{0}, n_boxes_{0};
  std::chrono::steady_clock::time_point last_report_;
  int frames_{0};
  double infer_ms_{0.0};
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  int rc = 0;
  try {
    auto node = std::make_shared<YoloNode>();
    while (rclcpp::ok()) {
      if (!node->Step()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
      rclcpp::spin_some(node);
    }
  } catch (const std::exception& e) {
    RCLCPP_FATAL(rclcpp::get_logger("yolo_node"), "%s", e.what());
    rc = rclcpp::ok() ? 1 : 0;
  }
  rclcpp::shutdown();
  return rc;
}
