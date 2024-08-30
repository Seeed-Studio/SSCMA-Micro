// #include <filesystem>

// #include "save.h"

// #ifndef NODE_SAVE_PATH_LOCAL
// #define NODE_SAVE_PATH_LOCAL \
//     "/home/lht/recamera/sscma-example-sg200x/components/sscma/test/build/local/"
// #endif

// #ifndef NODE_SAVE_PATH_EXTERNAL
// #define NODE_SAVE_PATH_EXTERNAL \
//     "/home/lht/recamera/sscma-example-sg200x/components/sscma/test/build/extern/"
// #endif

// #ifndef NODE_SAVE_MAX_SIZE
// #define NODE_SAVE_MAX_SIZE 128 * 1024 * 1024
// #endif

// namespace ma::node {

// static constexpr char TAG[] = "ma::node::save";

// SaveNode::SaveNode(std::string id)
//     : Node("save", id),
//       storage_(NODE_SAVE_PATH_LOCAL),
//       max_size_(NODE_SAVE_MAX_SIZE),
//       slice_(300),
//       camera_(nullptr),
//       thread_(nullptr) {}

// SaveNode::~SaveNode() {
//     if (thread_ != nullptr) {
//         delete thread_;
//     }
// }

// std::string SaveNode::generateFileName() {
//     auto now = std::time(nullptr);
//     std::ostringstream oss;
//     oss << storage_ << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S") << ".h264";
//     MA_LOGV(TAG, "filename: %s", oss.str().c_str());
//     return oss.str();
// }

// bool SaveNode::recycle() {
//     uint64_t total_size = 0;
//     std::vector<std::filesystem::directory_entry> files;

//     for (auto& p : std::filesystem::directory_iterator(storage_)) {
//         if (p.is_regular_file()) {
//             files.push_back(p);
//             total_size += p.file_size();
//         }
//     }

//     if (total_size > max_size_) {
//         std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
//             return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
//         });
//         for (const auto& file : files) {
//             uint64_t file_size = file.file_size();
//             if (std::filesystem::remove(file)) {
//                 MA_LOGV(TAG, "Deleted file: %s", file.path().c_str());
//                 total_size -= file_size;
//                 if (total_size <= max_size_) {
//                     break;
//                 }
//             }
//         }
//     }
//     return false;
// }

// void SaveNode::threadEntry() {
//     ma_tick_t start_ = 0;
//     while (camera_ == nullptr) {
//         if (!NodeFactory::getNodes("camera").empty()) {
//             camera_ = static_cast<CameraNode*>(NodeFactory::getNodes("camera")[0]);
//             break;
//         }
//         Thread::sleep(Tick::fromMilliseconds(100));
//     }

//     camera_->attach(CHN_H264, &frame_);

//     if (slice_ <= 0) {
//         file_.open(generateFileName(), std::ios::binary | std::ios::out);
//         recycle();
//     }

//     while (true) {
//         videoFrame* frame = nullptr;
//         try {
//             if (frame_.fetch(reinterpret_cast<void**>(&frame))) {
//                 if (slice_ > 0 && frame->timestamp >= start_ + Tick::fromSeconds(slice_)) {
//                     start_ = frame->timestamp;
//                     file_.flush();
//                     file_.close();
//                     file_.open(generateFileName(), std::ios::binary | std::ios::out);
//                     recycle();
//                 }
//                 file_.write(reinterpret_cast<char*>(frame->img.data), frame->img.size);
//                 frame->release();
//             }
//         } catch (std::exception& e) {
//             MA_LOGE(TAG, "exception: %s", e.what());
//         }
//     }
// }

// void SaveNode::threadEntryStub(void* obj) {
//     reinterpret_cast<SaveNode*>(obj)->threadEntry();
// }

// ma_err_t SaveNode::onCreate(const json& config, const Response& response) {
//     Guard guard(mutex_);
//     ma_err_t err = MA_OK;

//     try {

//         if (!config["data"].contains("config") || !config["data"]["config"].contains("slice") ||
//             !config["data"]["config"].contains("storage")) {
//             err = MA_EINVAL;
//             throw std::runtime_error("invalid config");
//         }

//         std::string storageType = config["data"]["config"]["storage"].get<std::string>();
//         storage_                = (storageType == "local") ? NODE_SAVE_PATH_LOCAL
//                            : (storageType == "external")   ? NODE_SAVE_PATH_EXTERNAL
//                                                            : "";

//         if (storage_.empty()) {
//             err = MA_ENOENT;
//             throw std::runtime_error("storage not exist");
//         }

//         if (!std::filesystem::exists(storage_) || !std::filesystem::is_directory(storage_)) {
//             err = MA_ENOENT;
//             throw std::runtime_error("storage not exist");
//         }
//     } catch (std::exception& e) {
//         MA_LOGE(TAG, "exception: %s", e.what());
//         response(json::object(
//             {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", e.what()}}));
//     }

//     slice_ = config["data"]["config"]["slice"].get<int>();

//     // TODO: max size should dynamic

//     MA_LOGV(TAG, "storage: %s, slice: %d max_size: %ld", storage_.c_str(), slice_, max_size_);

//     response(json::object(
//         {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));
//     thread_ = new Thread((type_ + "#" + id_).c_str(), threadEntryStub);
//     thread_->start(this);
//     return err;
// }

// ma_err_t SaveNode::onMessage(const json& message, const Response& response) {
//     Guard guard(mutex_);
//     ma_err_t err = MA_OK;
//     response(json::object(
//         {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));
//     return err;
// }

// ma_err_t SaveNode::onDestroy(const Response& response) {
//     Guard guard(mutex_);
//     ma_err_t err = MA_OK;

//     if (thread_ != nullptr) {
//         thread_->stop();
//     }

//     if (file_.is_open()) {
//         file_.close();
//     }

//     response(json::object(
//         {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));

//     return MA_OK;
// }

// REGISTER_NODE("save", SaveNode);

// }  // namespace ma::node
